#include "seasocks/connection.h"

#include "seasocks/credentials.h"
#include "seasocks/server.h"
#include "seasocks/stringutil.h"
#include "seasocks/logger.h"

#include "internal/Embedded.h"
#include "internal/LogStream.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <boost/lexical_cast.hpp>
#include <boost/noncopyable.hpp>
#include <boost/unordered_map.hpp>
#include <fstream>

#include "md5/md5.h"

namespace {

uint32_t parseWebSocketKey(const char* key) {
	uint32_t keyNumber = 0;
	uint32_t numSpaces = 0;
	for (;*key; ++key) {
		if (*key >= '0' && *key <= '9') {
			keyNumber = keyNumber * 10 + *key - '0';
		} else if (*key == ' ') {
			++numSpaces;
		}
	}
	return numSpaces > 0 ? keyNumber / numSpaces : 0;
}

char* extractLine(uint8_t*& first, uint8_t* last, char** colon = NULL) {
	uint8_t* ptr = first;
	for (uint8_t* ptr = first; ptr < last - 1; ++ptr) {
		if (ptr[0] == '\r' && ptr[1] == '\n') {
			ptr[0] = 0;
			uint8_t* result = first;
			first = ptr + 2;
			return reinterpret_cast<char*> (result);
		}
		if (colon && ptr[0] == ':' && *colon == NULL) {
			*colon = reinterpret_cast<char*> (ptr);
		}
	}
	return NULL;
}

std::string webtime(time_t time) {
	struct tm tm;
	gmtime_r(&time, &tm);
	char buf[1024];
	// Wed, 20 Apr 2011 17:31:28 GMT
	strftime(buf, sizeof(buf)-1, "%a, %d %b %Y %H:%M:%S %Z", &tm);
	return buf;
}

std::string now() {
	return webtime(time(NULL));
}

class RaiiFd : public boost::noncopyable {
	int _fd;
public:
	RaiiFd(const char* filename) {
		_fd = ::open(filename, O_RDONLY);
	}
	~RaiiFd() {
		if (_fd != -1) {
			::close(_fd);
		}
	}
	bool ok() const {
		return _fd != -1;
	}
	operator int() const {
		return _fd;
	}
};

const boost::unordered_map<std::string, std::string> contentTypes = {
	{ "txt", "text/plain" },
	{ "css", "text/css" },
	{ "csv", "text/csv" },
	{ "htm", "text/html" },
	{ "html", "text/html" },
	{ "xml", "text/xml" },
	{ "js", "text/javascript" }, // Technically it should be application/javascript (RFC 4329), but IE8 struggles with that
	{ "xhtml", "application/xhtml+xml" },
	{ "json", "application/json" },
	{ "pdf", "application/pdf" },
	{ "zip", "application/zip" },
	{ "tar", "application/x-tar" },
	{ "gif", "image/gif" },
	{ "jpeg", "image/jpeg" },
	{ "jpg", "image/jpeg" },
	{ "tiff", "image/tiff" },
	{ "tif", "image/tiff" },
	{ "png", "image/png" },
	{ "svg", "image/svg+xml" },
	{ "ico", "image/x-icon" },
	{ "swf", "application/x-shockwave-flash" },
	{ "mp3", "audio/mpeg" },
	{ "wav", "audio/x-wav" },
};

std::string getExt(const std::string& path) {
	auto lastDot = path.find_last_of('.');
	if (lastDot != path.npos) {
		return path.substr(lastDot + 1);
	}
	return "";
}

const std::string& getContentType(const std::string& path) {
	auto it = contentTypes.find(getExt(path));
	if (it != contentTypes.end()) {
		return it->second;
	}
	static const std::string defaultType("text/html");
	return defaultType;
}

// Cacheability is only set for resources that *REQUIRE* caching for browser support reasons.
// It's off for everything else to save on browser reload headaches during development, at
// least until we support ETags or If-Modified-Since: type checking, which we may never do.
bool isCacheable(const std::string& path) {
	std::string extension = getExt(path);
	if (extension == "mp3" || extension == "wav") {
		return true;
	}
	return false;
}

const size_t MaxBufferSize = 16 * 1024 * 1024;
const size_t ReadWriteBufferSize = 16 * 1024;
const size_t MaxWebsocketMessageSize = 16384;
const size_t MaxHeadersSize = 64 * 1024;

class PrefixWrapper : public SeaSocks::Logger {
	std::string _prefix;
	boost::shared_ptr<Logger> _logger;
public:
	PrefixWrapper(const std::string& prefix, boost::shared_ptr<Logger> logger) :
		_prefix(prefix), _logger(logger) {}

	virtual void log(Level level, const char* message) {
		_logger->log(level, (_prefix + message).c_str());
	}
};

}  // namespace

namespace SeaSocks {

Connection::Connection(
		boost::shared_ptr<Logger> logger,
		Server* server,
		int fd,
		const sockaddr_in& address,
		boost::shared_ptr<SsoAuthenticator> sso)
	: _logger(new PrefixWrapper(formatAddress(address) + " : ", logger)),
	  _server(server),
	  _fd(fd),
	  _state(READING_HEADERS),
	  _closeOnEmpty(false),
	  _registeredForWriteEvents(false),
	  _bytesSent(0),
	  _bytesReceived(0),
	  _address(address),
	  _sso(sso),
	  _credentials(boost::shared_ptr<Credentials>(new Credentials())) {
	assert(server->getStaticPath() != "");
	_webSocketKeys[0] = _webSocketKeys[1] = 0;
}

Connection::~Connection() {
	close();
}

void Connection::close() {
	if (_webSocketHandler) {
		_webSocketHandler->onDisconnect(this);
		_webSocketHandler.reset();
	}
	if (_fd != -1) {
		_server->remove(this);
		::close(_fd);
	}
	_fd = -1;
}

int Connection::safeSend(const void* data, size_t size) {
	int sendResult = ::send(_fd, data, size, MSG_NOSIGNAL);
	if (sendResult == -1) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			// Treat this as if zero bytes were written.
			return 0;
		}
	} else {
		_bytesSent += sendResult;
	}
	return sendResult;
}

bool Connection::write(const void* data, size_t size, bool flushIt) {
	if (closed()) {
		return false;
	}
	assert(!_closeOnEmpty);
	if (size == 0) {
		return true;
	}
	int bytesSent = 0;
	if (_outBuf.empty() && flushIt) {
		// Attempt fast path, send directly.
		bytesSent = safeSend(data, size);
		if (bytesSent == static_cast<int>(size)) {
			// We sent directly.
			return true;
		}
		if (bytesSent == -1) {
			LS_ERROR(_logger, "Unable to write to socket: " << getLastError());
			return false;
		}
	}
	size_t bytesToBuffer = size - bytesSent;
	size_t endOfBuffer = _outBuf.size();
	size_t newBufferSize = endOfBuffer + bytesToBuffer;
	if (newBufferSize >= MaxBufferSize) {
		LS_WARNING(_logger, "Closing connection: buffer size too large (" << newBufferSize << " > " << MaxBufferSize);
		close();
		return false;
	}
	_outBuf.resize(newBufferSize);
	memcpy(&_outBuf[endOfBuffer], reinterpret_cast<const uint8_t*>(data) + bytesSent, bytesToBuffer);
	if (flushIt) {
		flush();
	}
	return true;
}

bool Connection::bufferLine(const char* line) {
	static const char crlf[] = { '\r', '\n' };
	if (!write(line, strlen(line), false)) return false;
	return write(crlf, 2, false);
}

bool Connection::bufferLine(const std::string& line) {
	std::string lineAndCrlf = line + "\r\n";
	return write(lineAndCrlf.c_str(), lineAndCrlf.length(), false);
}

bool Connection::handleDataReadyForRead() {
	if (closed()) {
		return false;
	}
	size_t curSize = _inBuf.size();
	_inBuf.resize(curSize + ReadWriteBufferSize);
	int result = ::read(_fd, &_inBuf[curSize], ReadWriteBufferSize);
	if (result == -1) {
		LS_ERROR(_logger, "Unable to read from socket : " << getLastError());
		return false;
	}
	if (result == 0) {
		LS_INFO(_logger, "Remote end closed connection");
		return false;
	}
	_bytesReceived += result;
	_inBuf.resize(curSize + result);
	if (!handleNewData()) {
		return false;
	}
	return checkCloseConditions();
}

bool Connection::handleDataReadyForWrite() {
	if (closed()) {
		return false;
	}
	if (!flush()) {
		return false;
	}
	return checkCloseConditions();
}

bool Connection::flush() {
	if (_outBuf.empty()) {
		return true;
	}
	int numSent = safeSend(&_outBuf[0], _outBuf.size());
	if (numSent == -1) {
		LS_ERROR(_logger, "Unable to write to socket : " << getLastError());
		return false;
	}
	_outBuf.erase(_outBuf.begin(), _outBuf.begin() + numSent);
	if (_outBuf.size() > 0 && !_registeredForWriteEvents) {
		if (!_server->subscribeToWriteEvents(this)) {
			return false;
		}
		_registeredForWriteEvents = true;
	} else if (_outBuf.empty() && _registeredForWriteEvents) {
		if (!_server->unsubscribeFromWriteEvents(this)) {
			return false;
		}
		_registeredForWriteEvents = false;
	}
	return true;
}

bool Connection::closed() const {
	return _fd == -1;
}

bool Connection::checkCloseConditions() {
	if (closed()) {
		return true;
	}
	if (_closeOnEmpty && _outBuf.empty()) {
		LS_DEBUG(_logger, "Closing, now empty");
		close();
		return false;
	}
	return true;
}

bool Connection::handleNewData() {
	switch (_state) {
	case READING_HEADERS:
		return handleHeaders();
	case READING_WEBSOCKET_KEY3:
		return handleWebSocketKey3();
	case HANDLING_WEBSOCKET:
		return handleWebSocket();
	default:
		assert(false);
		return false;
	}
}

bool Connection::handleHeaders() {
	if (_inBuf.size() < 4) {
		return true;
	}
	for (size_t i = 0; i <= _inBuf.size() - 4; ++i) {
		if (_inBuf[i] == '\r' &&
			_inBuf[i+1] == '\n' &&
			_inBuf[i+2] == '\r' &&
			_inBuf[i+3] == '\n') {
			if (!processHeaders(&_inBuf[0], &_inBuf[i + 2])) {
				return false;
			}
			_inBuf.erase(_inBuf.begin(), _inBuf.begin() + i + 4);
			return handleNewData();
		}
	}
	if (_inBuf.size() > MaxHeadersSize) {
		return sendUnsupportedError("Headers too big");
	}
	return true;
}

bool Connection::handleWebSocketKey3() {
	if (_inBuf.size() < 8) {
		return true;
	}

	struct {
		uint32_t key0;
		uint32_t key1;
		char key2[8];
	} md5Source;

	md5Source.key0 = htonl(_webSocketKeys[0]);
	md5Source.key1 = htonl(_webSocketKeys[1]);
	memcpy(&md5Source.key2, &_inBuf[0], 8);

	uint8_t digest[16];
	md5_state_t md5state;
	md5_init(&md5state);
	md5_append(&md5state, reinterpret_cast<const uint8_t*>(&md5Source), sizeof(md5Source));
	md5_finish(&md5state, digest);

	LS_DEBUG(_logger, "Attempting websocket upgrade");

	bufferLine("HTTP/1.1 101 WebSocket Protocol Handshake");
	bufferLine("Upgrade: WebSocket");
	bufferLine("Connection: Upgrade");
	write(&_webSockExtraHeaders[0], _webSockExtraHeaders.size(), false);
	bufferLine("");

	write(&digest, 16, true);

	_state = HANDLING_WEBSOCKET;
	_inBuf.erase(_inBuf.begin(), _inBuf.begin() + 8);
	if (_webSocketHandler) {
		_webSocketHandler->onConnect(this);
	}

	return true;
}

bool Connection::send(const char* webSocketResponse) {
	uint8_t zero = 0;
	if (!write(&zero, 1, false)) return false;
	if (!write(webSocketResponse, strlen(webSocketResponse), false)) return false;
	uint8_t effeff = 0xff;
	return write(&effeff, 1, true);
}

boost::shared_ptr<Credentials> Connection::credentials() {
	return _credentials;
}

bool Connection::handleWebSocket() {
	if (_inBuf.empty()) {
		return true;
	}
	size_t firstByteNotConsumed = 0;
	size_t messageStart = 0;
	while (messageStart < _inBuf.size()) {
		if (_inBuf[messageStart] != 0) {
			LS_ERROR(_logger, "Error in WebSocket input stream (got " << (int)_inBuf[messageStart] << ")");
			return false;
		}
		// TODO: UTF-8
		size_t endOfMessage = 0;
		for (size_t i = messageStart + 1; i < _inBuf.size(); ++i) {
			if (_inBuf[i] == 0xff) {
				endOfMessage = i;
				break;
			}
		}
		if (endOfMessage != 0) {
			_inBuf[endOfMessage] = 0;
			if (!handleWebSocketMessage(reinterpret_cast<const char*>(&_inBuf[messageStart + 1]))) {
				return false;
			}
			firstByteNotConsumed = endOfMessage + 1;
		} else {
			break;
		}
	}
	if (firstByteNotConsumed != 0) {
		_inBuf.erase(_inBuf.begin(), _inBuf.begin() + firstByteNotConsumed);
	}
	if (_inBuf.size() > MaxWebsocketMessageSize) {
		LS_ERROR(_logger, "WebSocket message too long");
		close();
		return false;
	}
	return true;
}

bool Connection::handleWebSocketMessage(const char* message) {
	LS_DEBUG(_logger, "Got web socket message: '" << message << "'");
	if (_webSocketHandler) {
		_webSocketHandler->onData(this, message);
	}
	return true;
}

bool Connection::sendError(int errorCode, const char* message, const char* body) {
	assert(_state != HANDLING_WEBSOCKET);
	LS_INFO(_logger, "Sending error " << errorCode << " - " << message << " (" << body << ")");
	bufferLine("HTTP/1.1 " + boost::lexical_cast<std::string>(errorCode) + std::string(" ") + message);
	std::stringstream documentStr;
	documentStr << "<html><head><title>" << errorCode << " - " << message << "</title></head>"
			<< "<body><h1>" << errorCode << " - " << message << "</h1>"
			<< "<div>" << body << "</div><hr/><div><i>Powered by SeaSocks</i></div></body></html>";
	std::string document(documentStr.str());
	bufferLine("Content-Length: " + boost::lexical_cast<std::string>(document.length()));
	bufferLine("Connection: close");
	bufferLine("");
	bufferLine(document);
	flush();
	_closeOnEmpty = true;
	return true;
}

bool Connection::sendUnsupportedError(const char* reason) {
	return sendError(501, "Not Implemented", reason);
}

bool Connection::send404(const std::string& path) {
	auto embedded = findEmbeddedContent(path);
	if (embedded) {
		return sendData(getContentType(path), embedded->data, embedded->length);
	} else if (strcmp(path.c_str(), "/_livestats.js") == 0) {
		auto stats = _server->getStatsDocument();
		return sendData("text/javascript", stats.c_str(), stats.length());
	} else {
		return sendError(404, "Not Found", path.c_str());
	}
}

bool Connection::sendBadRequest(const char* reason) {
	return sendError(400, "Bad Request", reason);
}

bool Connection::processHeaders(uint8_t* first, uint8_t* last) {
	char* requestLine = extractLine(first, last);
	assert(requestLine != NULL);

	const char* verb = shift(requestLine);
	if (verb == NULL) {
		return sendBadRequest("Malformed request line");
	}
	if (strcmp(verb, "GET") != 0) {
		return sendUnsupportedError("We only support GET");
	}
	const char* requestUri = shift(requestLine);
	if (requestUri == NULL) {
		return sendBadRequest("Malformed request line");
	}
	_requestUri = std::string(requestUri);
	
	const char* httpVersion = shift(requestLine);
	if (httpVersion == NULL) {
		return sendBadRequest("Malformed request line");
	}
	if (strcmp(httpVersion, "HTTP/1.1") != 0) {
		return sendUnsupportedError("Unsupported HTTP version");
	}
	if (*requestLine != 0) {
		return sendBadRequest("Trailing crap after http version");
	}

	bool haveConnectionUpgrade = false;
	bool haveWebSocketUprade = false;
	bool allowCrossOrigin = _server->isCrossOriginAllowed(requestUri);
	std::string host;
	std::string cookie;
	std::string rangeHeader;
	while (first < last) {
		char* colonPos = NULL;
		char* headerLine = extractLine(first, last, &colonPos);
		assert(headerLine != NULL);
		if (colonPos == NULL) {
			return sendBadRequest("Malformed header");
		}
		*colonPos = 0;
		const char* key = headerLine;
		const char* value = skipWhitespace(colonPos + 1);
		LS_DEBUG(_logger, "Key: " << key << " || " << value);
		std::string strValue(value);
		if (strcasecmp(key, "Connection") == 0) {
			if (strcasecmp(value, "upgrade") == 0) {
				haveConnectionUpgrade = true;
			}
		} else if (strcasecmp(key, "Upgrade") == 0 && strcasecmp(value, "websocket") == 0) {
			haveWebSocketUprade = true;
		} else if (strcasecmp(key, "Sec-WebSocket-Key1") == 0) {
			_webSocketKeys[0] = parseWebSocketKey(value);
		} else if (strcasecmp(key, "Sec-WebSocket-Key2") == 0) {
			_webSocketKeys[1] = parseWebSocketKey(value);
		} else if (strcasecmp(key, "Origin") == 0 && allowCrossOrigin) {
			_webSockExtraHeaders += "Sec-WebSocket-Origin: " + strValue + "\r\n";
		} else if (strcasecmp(key, "Host") == 0) {
			if (!allowCrossOrigin) {
				_webSockExtraHeaders += "Sec-WebSocket-Origin: http://" + strValue + "\r\n";
			}
			_webSockExtraHeaders += "Sec-WebSocket-Location: ws://" + strValue + requestUri;
			_webSockExtraHeaders += "\r\n";
			host = strValue;
		} else if (strcasecmp(key, "Cookie") == 0) {
			cookie = strValue;
		} else if (strcasecmp(key, "Range") == 0) {
			rangeHeader = strValue;
		}
	}

	// <SSO>
	if (_sso) {
		if (_sso->isBounceBackFromSsoServer(requestUri)) {
			if (_sso->validateSignature(requestUri)) {
				std::stringstream response;
				std::string error;
				if(_sso->respondWithLocalCookieAndRedirectToOriginalPage(requestUri, response, error)) {
					std::string content = response.str();
					bool result = write(content.c_str(), content.length(), true);
					_closeOnEmpty = true;
					return result;
				} else {
					return sendError(500, error.c_str(), requestUri);
				}
			} else {
				return sendError(500, "Invalid SSO signature", requestUri);
			}
		}

		if (_sso->enabledForPath(requestUri)) {
			_sso->extractCredentialsFromLocalCookie(cookie, _credentials);
			if (!_credentials->authenticated) {
				if (_sso->requestExplicityForbidsDrwSsoRedirect()) {
					return sendError(403, "Not Authorized", requestUri);
				} else {
					std::stringstream response;
					std::string error;
					if (_sso->respondWithRedirectToAuthenticationServer(requestUri, host, response, error)) {
						std::string content = response.str();
						bool result = write(content.c_str(), content.length(), true);
						_closeOnEmpty = true;
						return result;
					} else {
						return sendError(500, error.c_str(), requestUri);
					}
				}
			}
		}
	}
	// </SSO>

	if (haveConnectionUpgrade && haveWebSocketUprade) {
		LS_DEBUG(_logger, "Got a websocket with key1=0x" << std::hex << _webSocketKeys[0] << ", key2=0x" << _webSocketKeys[1]);
		_webSocketHandler = _server->getWebSocketHandler(requestUri);
		if (!_webSocketHandler) {
			LS_ERROR(_logger, "Couldn't find WebSocket end point for '" << requestUri << "'");
			return send404(requestUri);
		}
		_state = READING_WEBSOCKET_KEY3;
		return true;
	} else {
		return sendStaticData(requestUri, rangeHeader);
	}
}

bool Connection::parseRange(const std::string& rangeStr, Range& range) const {
	size_t minusPos = rangeStr.find('-');
	if (minusPos == std::string::npos) {
		LS_ERROR(_logger, "Bad range: '" << rangeStr << "'");
		return false;
	}
	if (minusPos == 0) {
		// A range like "-500" means 500 bytes from end of file to end.
		range.start = boost::lexical_cast<int>(rangeStr);
		range.end = std::numeric_limits<long>::max();
		return true;
	} else {
		range.start = boost::lexical_cast<int>(rangeStr.substr(0, minusPos));
		if (minusPos == rangeStr.size()-1) {
			range.end = std::numeric_limits<long>::max();
		} else {
			range.end = boost::lexical_cast<int>(rangeStr.substr(minusPos + 1));
		}
		return true;
	}
	return false;
}

bool Connection::parseRanges(const std::string& range, std::list<Range>& ranges) const {
	static const std::string expectedPrefix = "bytes=";
	if (range.length() < expectedPrefix.length() || range.substr(0, expectedPrefix.length()) != expectedPrefix) {
		LS_ERROR(_logger, "Bad range request prefix: '" << range << "'");
		return false;
	}
	auto rangesText = split(range.substr(expectedPrefix.length()), ',');
	for (auto it = rangesText.begin(); it != rangesText.end(); ++it) {
		Range r;
		if (!parseRange(*it, r)) {
			return false;
		}
		ranges.push_back(r);
	}
	return !ranges.empty();
}

// Sends HTTP 200 or 206, content-length, and range info as needed. Returns the actual file ranges
// needing sending.
std::list<Connection::Range> Connection::processRangesForStaticData(const std::list<Range>& origRanges, long fileSize) {
	if (origRanges.empty()) {
		// Easy case: a non-range request.
		bufferLine("HTTP/1.1 200 OK");
		bufferLine("Content-Length: " + boost::lexical_cast<std::string>(fileSize));
		return { Range { 0, fileSize - 1 } };
	}

	// Partial content request.
	bufferLine("HTTP/1.1 206 OK");
	int contentLength = 0;
	std::ostringstream rangeLine;
	rangeLine << "Content-Range: ";
	std::list<Range> sendRanges;
	for (auto rangeIter = origRanges.cbegin(); rangeIter != origRanges.cend(); ++rangeIter) {
		Range actualRange = *rangeIter;
		if (actualRange.start < 0) {
			actualRange.start += fileSize;
		}
		if (actualRange.start >= fileSize) {
			actualRange.start = fileSize - 1;
		}
		if (actualRange.end >= fileSize) {
			actualRange.end = fileSize - 1;
		}
		contentLength += actualRange.length();
		sendRanges.push_back(actualRange);
		rangeLine << actualRange.start << "-" << actualRange.end;
	}
	rangeLine << "/" << fileSize;
	bufferLine(rangeLine.str());
	bufferLine("Content-Length: " + boost::lexical_cast<std::string>(contentLength));
	return sendRanges;
}

bool Connection::sendStaticData(const char* requestUri, const std::string& rangeHeader) {
	std::string path = _server->getStaticPath() + requestUri;
	// Trim any trailing queries.
	size_t queryPos = path.find('?');
	if (queryPos != path.npos) {
		path.resize(queryPos);
	}
	if (*path.rbegin() == '/') {
		path += "index.html";
	}
	RaiiFd input(path.c_str());
	struct stat stat;
	if (!input.ok() || ::fstat(input, &stat) == -1) {
		return send404(requestUri);
	}
	std::list<Range> ranges;
	if (!rangeHeader.empty() && !parseRanges(rangeHeader, ranges)) {
		return sendBadRequest("Bad range header");
	}
	ranges = processRangesForStaticData(ranges, stat.st_size);
	bufferLine("Content-Type: " + getContentType(path));
	bufferLine("Connection: close");
	bufferLine("Server: SeaSocks");
	bufferLine("Accept-Ranges: bytes");
	auto nowTime = now();
	bufferLine("Date: " + nowTime);
	bufferLine("Last-Modified: " + webtime(stat.st_mtime));
	if (!isCacheable(path)) {
		bufferLine("Cache-Control: no-store");
		bufferLine("Pragma: no-cache");
		bufferLine("Expires: " + nowTime);
	}
	bufferLine("");
	flush();

	for (auto rangeIter = ranges.cbegin(); rangeIter != ranges.cend(); ++rangeIter) {
		if (::lseek(input, rangeIter->start, SEEK_SET) == -1) {
			// We've (probably) already sent data.
			return false;
		}
		auto bytesLeft = rangeIter->length();
		while (bytesLeft) {
			char buf[ReadWriteBufferSize];
			auto bytesRead = ::read(input, buf, std::min(sizeof(buf), bytesLeft));
			if (bytesRead <= 0) {
				const static std::string unexpectedEof("Unexpected EOF");
				LS_ERROR(_logger, "Error reading file: " << bytesRead == 0 ? unexpectedEof : getLastError());
				// We can't send an error document as we've sent the header.
				return false;
			}
			bytesLeft -= bytesRead;
			if (!write(buf, bytesRead, true)) {
				return false;
			}
		}
	}
	_closeOnEmpty = true;
	return true;
}
bool Connection::sendData(const std::string& type, const char* start, size_t size) {
	bufferLine("HTTP/1.1 200 OK");
	bufferLine("Content-Type: " + type);
	bufferLine("Content-Length: " + boost::lexical_cast<std::string>(size));
	bufferLine("Server: SeaSocks");
	bufferLine("Connection: close");
	bufferLine("");
	bool result = write(start, size, true);
	_closeOnEmpty = true;
	return result;
}

}  // SeaSocks
