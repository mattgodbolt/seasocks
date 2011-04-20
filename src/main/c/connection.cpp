#include "seasocks/connection.h"

#include "seasocks/credentials.h"
#include "seasocks/server.h"
#include "seasocks/stringutil.h"
#include "seasocks/logger.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <sstream>

#include <boost/lexical_cast.hpp>
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

std::vector<std::string> split(const std::string& input, char splitChar) {
	std::vector<std::string> result;
	size_t pos = 0;
	size_t newPos;
	while ((newPos = input.find(splitChar, pos)) != std::string::npos) {
		result.push_back(input.substr(pos, newPos - pos));
		pos = newPos + 1;
	}
	result.push_back(input.substr(pos));
	return result;
}

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

const std::string& getContentType(const std::string& path) {
	auto lastDot = path.find_last_of('.');
	if (lastDot != path.npos) {
		std::string extension = path.substr(lastDot + 1);
		auto it = contentTypes.find(extension);
		if (it != contentTypes.end()) {
			return it->second;
		}
	}
	static const std::string defaultType("text/html");
	return defaultType;
}

const size_t MaxBufferSize = 16 * 1024 * 1024;
const size_t ReadWriteBufferSize = 16 * 1024;
const size_t MaxWebsocketMessageSize = 16384;
const size_t MaxHeadersSize = 64 * 1024;

}  // namespace

namespace SeaSocks {

Connection::Connection(
		boost::shared_ptr<Logger> logger,
		Server* server,
		int fd,
		const sockaddr_in& address,
		boost::shared_ptr<SsoAuthenticator> sso)
	: _logger(logger),
	  _server(server),
	  _fd(fd),
	  _state(READING_HEADERS),
	  _closeOnEmpty(false),
	  _registeredForWriteEvents(false),
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
		_server->unsubscribeFromAllEvents(this);
		::close(_fd);
	}
	_fd = -1;
}

int Connection::safeSend(const void* data, size_t size) const {
	int sendResult = ::send(_fd, data, size, MSG_NOSIGNAL);
	if (sendResult == -1) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			// Treat this as if zero bytes were written.
			return 0;
		}
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
			_logger->error("Unable to write to socket: %s", getLastError());
			return false;
		}
	}
	size_t bytesToBuffer = size - bytesSent;
	size_t endOfBuffer = _outBuf.size();
	size_t newBufferSize = endOfBuffer + bytesToBuffer;
	if (newBufferSize >= MaxBufferSize) {
		_logger->warning("Closing connection to %s: buffer size too large (%d>%d)",
				formatAddress(_address), newBufferSize, MaxBufferSize);
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
		_logger->error("Unable to read from socket: %s", getLastError());
		return false;
	}
	if (result == 0) {
		_logger->info("Remote end closed connection");
		return false;
	}
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
		_logger->error("%s : Unable to write to socket: %s", formatAddress(_address), getLastError());
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
		_logger->debug("%s : Closing, now empty", formatAddress(_address));
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

	_logger->debug("%s : Attempting websocket upgrade", formatAddress(_address));

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
			_logger->error("%s : Error in WebSocket input stream (got 0x%02x)", formatAddress(_address), _inBuf[messageStart]);
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
		_logger->error("%s : WebSocket message too long", formatAddress(_address));
		close();
		return false;
	}
	return true;
}

bool Connection::handleWebSocketMessage(const char* message) {
	_logger->debug("%s : Got web socket message: '%s'", formatAddress(_address), message);
	if (_webSocketHandler) {
		_webSocketHandler->onData(this, message);
	}
	return true;
}

bool Connection::sendError(int errorCode, const char* message, const char* body) {
	assert(_state != HANDLING_WEBSOCKET);
	_logger->info("%s : Sending error %d - %s (%s)", formatAddress(_address), errorCode, message, body);
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

bool Connection::send404(const char* path) {
	if (strcmp(path, "/favicon.ico") == 0) {
		return sendDefaultFavicon();
	} else {
		return sendError(404, "Not Found", path);
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

	bool keepAlive = false;
	bool haveConnectionUpgrade = false;
	bool haveWebSocketUprade = false;
	bool allowCrossOrigin = _server->isCrossOriginAllowed(requestUri);
	std::string host;
	std::string cookie;
	std::list<Range> ranges;
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
		_logger->debug("Key: %s || Value: %s", key, value);
		std::string strValue(value);
		if (strcasecmp(key, "Connection") == 0) {
			if (strcasecmp(value, "keep-alive") == 0) {
				keepAlive = true;
			} else if (strcasecmp(value, "upgrade") == 0) {
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
			if (!parseRanges(strValue, ranges)) {
				return sendBadRequest("Bad range header");
			}
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
					return write(content.c_str(), content.length(), true);
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
						return write(content.c_str(), content.length(), true);
					} else {
						return sendError(500, error.c_str(), requestUri);
					}
				}
			}
		}
	}
	// </SSO>

	if (haveConnectionUpgrade && haveWebSocketUprade) {
		_logger->debug("%s : Got a websocket with key1=0x%08x, key2=0x%08x", formatAddress(_address), _webSocketKeys[0], _webSocketKeys[1]);
		_webSocketHandler = _server->getWebSocketHandler(requestUri);
		if (!_webSocketHandler) {
			_logger->error("%s : Couldn't find WebSocket end point for '%s'", formatAddress(_address), requestUri);
			return send404(requestUri);
		}
		_state = READING_WEBSOCKET_KEY3;
		return true;
	} else {
		return sendStaticData(keepAlive, requestUri, ranges);
	}
}

bool Connection::parseRange(const std::string& rangeStr, Range& range) const {
	size_t minusPos = rangeStr.find('-');
	if (minusPos == std::string::npos) {
		_logger->error("Bad range: '%s'", rangeStr.c_str());
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
		_logger->error("Bad range request prefix: '%s'", range.c_str());
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

bool Connection::sendStaticData(bool keepAlive, const char* requestUri, const std::list<Range>& origRanges) {
	std::string path = _server->getStaticPath() + requestUri;
	// Trim any trailing queries.
	size_t queryPos = path.find('?');
	if (queryPos != path.npos) {
		path.resize(queryPos);
	}
	if (*path.rbegin() == '/') {
		path += "index.html";
	}
	std::ifstream input(path, std::ios::in | std::ios::binary);
	if (!input) {
		return send404(requestUri);
	}
	input.seekg(0, std::ios::end);
	auto fileSize = static_cast<std::streamoff>(input.tellg());
	std::list<Range> sendRanges;
	if (!origRanges.empty()) {
		bufferLine("HTTP/1.1 206 OK");
		int contentLength = 0;
		std::ostringstream rangeLine;
		rangeLine << "Content-Range: ";
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
	} else {
		bufferLine("HTTP/1.1 200 OK");
		bufferLine("Content-Length: " + boost::lexical_cast<std::string>(fileSize));
		sendRanges.push_back(Range { 0, fileSize - 1 });
	}
	char buf[ReadWriteBufferSize];
	bufferLine("Content-Type: " + getContentType(path));
	if (keepAlive) {
		bufferLine("Connection: keep-alive");
	} else {
		bufferLine("Connection: close");
	}
	bufferLine("");
	flush();

	for (auto rangeIter = sendRanges.cbegin(); rangeIter != sendRanges.cend(); ++rangeIter) {
		input.seekg(rangeIter->start);
		if (input.fail()) {
			// We've (probably) already sent data.
			return false;
		}
		_logger->info("Sending range %d-%d", rangeIter->start, rangeIter->end);
		auto bytesLeft = rangeIter->length();
		_logger->info("Length of block: %d", bytesLeft);
		while (bytesLeft) {
			_logger->info("Bytes left: %d", bytesLeft);
			input.read(buf, std::min(sizeof(buf), bytesLeft));
			if (input.fail() && !input.eof()) {
				_logger->error("%s : Error reading file", formatAddress(_address));
				// We can't send an error document as we've sent the header.
				return false;
			}
			auto bytesRead = input.gcount();
			_logger->info("Bytes read: %d", bytesRead);
			bytesLeft -= bytesRead;
			if (!write(buf, bytesRead, true)) {
				return false;
			}
		}
	}
	if (!keepAlive) {
		_logger->debug("%s : Closing on empty", formatAddress(_address));
		_closeOnEmpty = true;
	}
	return true;
}

bool Connection::sendDefaultFavicon() {
	static const int dataLength = 1406;
	static const char data[] = // if changing this, remember to change the length above.
		"\x00\x00\x01\x00\x01\x00\x10\x10\x00\x00\x01\x00\x08\x00\x68\x05\x00\x00\x16\x00"
		"\x00\x00\x28\x00\x00\x00\x10\x00\x00\x00\x20\x00\x00\x00\x01\x00\x08\x00\x00\x00" 
		"\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00" 
		"\x00\x00\xE3\xE2\xE4\x00\x24\x29\x4D\x00\xF2\xF2\xF0\x00\x6F\x7F\xB1\x00\x84\x83" 
		"\x8A\x00\x4B\x5B\x7F\x00\x6F\x70\x6E\x00\xF7\xF7\xFC\x00\x8B\x8D\x8E\x00\xCF\xD2" 
		"\xD0\x00\xDE\xDE\xDC\x00\x32\x37\x55\x00\xC9\xC8\xCA\x00\xFB\xFB\xFD\x00\xC3\xC6" 
		"\xC4\x00\xBF\xC2\xC0\x00\x4F\x5B\x91\x00\xEF\xEF\xF1\x00\xEE\xEF\xF0\x00\x23\x26" 
		"\x3C\x00\xCB\xCA\xC9\x00\x74\x85\xD5\x00\xEF\xEA\xE7\x00\xEA\xEB\xEC\x00\xFA\xF9" 
		"\xF9\x00\x7E\x95\xDC\x00\x17\x1C\x30\x00\xE0\xE3\xE2\x00\xF5\xF1\xF4\x00\xF4\xF1" 
		"\xF3\x00\x68\x62\x61\x00\x6A\x7A\xC1\x00\x71\x83\xAA\x00\xDF\xDF\xE1\x00\x72\x72" 
		"\x7C\x00\x3D\x3D\x46\x00\x2A\x2B\x36\x00\xC7\xC7\xCC\x00\x2A\x32\x51\x00\x6C\x6B" 
		"\x6C\x00\xD1\xD3\xD3\x00\x50\x61\x8C\x00\x53\x4E\x4C\x00\x9A\xA2\xC0\x00\x68\x7B" 
		"\xB2\x00\xAF\xB4\xBE\x00\x3A\x41\x65\x00\xEA\xE5\xE6\x00\x53\x5B\xA0\x00\x8C\xA4" 
		"\xEB\x00\xD2\xD1\xD1\x00\xB8\xB9\xBA\x00\xFE\xFE\xFE\x00\x7B\x87\xBF\x00\x69\x6E" 
		"\x6D\x00\xE4\xE8\xE7\x00\xF8\xF8\xF8\x00\x9D\x9F\xA2\x00\x3B\x40\x59\x00\xF6\xF6" 
		"\xF6\x00\x50\x5C\x90\x00\x8A\xA4\xE6\x00\xF2\xF2\xF2\x00\xDE\xE0\xE1\x00\x5F\x6F" 
		"\x92\x00\xC4\xC6\xCA\x00\xB7\xB4\xAC\x00\x34\x34\x3E\x00\x9C\x9F\x9E\x00\x5E\x6B" 
		"\xA2\x00\x32\x37\x57\x00\xEE\xEC\xEB\x00\x9D\xA3\xB0\x00\xEC\xEA\xE9\x00\xD7\xDA" 
		"\xD7\x00\xA1\xA5\xA0\x00\xE6\xE8\xE3\x00\x7E\x8F\xE1\x00\x32\x3B\x68\x00\xBE\xBC" 
		"\xC1\x00\xCE\xD2\xCE\x00\xDC\xDA\xD9\x00\xF7\xF7\xF8\x00\xD2\xD0\xCF\x00\x68\x64" 
		"\x63\x00\xEF\xEF\xF0\x00\xED\xED\xEE\x00\x49\x56\x80\x00\x3A\x43\x6A\x00\x9F\xA2" 
		"\xA2\x00\x54\x44\x3E\x00\x7F\x96\xD2\x00\xBE\xBF\xC5\x00\x87\x8C\x8D\x00\x7D\x83" 
		"\xA1\x00\x83\x93\xB8\x00\xD6\xD9\xD7\x00\xD7\xD7\xD8\x00\x99\xA0\xAD\x00\xD4\xD7" 
		"\xD5\x00\xD2\xD5\xD3\x00\xAC\xAF\xB3\x00\x46\x53\x70\x00\x50\x65\x8B\x00\xDE\xE1" 
		"\xDC\x00\x74\x75\x70\x00\x4C\x53\x87\x00\xDA\xD9\xD8\x00\x37\x44\x6B\x00\x54\x61" 
		"\xA0\x00\x56\x62\x98\x00\x73\x84\xC3\x00\xD3\xD5\xD1\x00\x3D\x3E\x49\x00\x4F\x5C" 
		"\x7D\x00\xAC\xA3\x9C\x00\x32\x34\x3E\x00\xAD\xB1\xAE\x00\x15\x19\x2E\x00\x7B\x8D" 
		"\xCF\x00\x4F\x5B\x84\x00\xDC\xDE\xDE\x00\x13\x15\x2C\x00\x22\x24\x42\x00\x1F\x1D" 
		"\x35\x00\x18\x20\x38\x00\x29\x2B\x50\x00\x99\x9D\x9A\x00\xF3\xF4\xEF\x00\x3E\x4F" 
		"\x73\x00\xBA\xBC\xBF\x00\xAF\xB7\xD2\x00\x45\x4B\x66\x00\xCA\xCC\xCC\x00\x16\x18" 
		"\x22\x00\x67\x75\xBB\x00\x8D\x91\xA2\x00\xEE\xEC\xEA\x00\x90\x93\x91\x00\xC4\xC6" 
		"\xC6\x00\x6C\x81\xBD\x00\x45\x4E\x6D\x00\xEE\xEF\xF1\x00\xFF\xFF\xFF\x00\xFD\xFD" 
		"\xFD\x00\xFB\xFB\xFB\x00\xC1\xC5\xCA\x00\x63\x78\xAA\x00\xB6\xAF\xAE\x00\x44\x52" 
		"\x7D\x00\xB0\xAE\xB2\x00\xF2\xF3\xF2\x00\xCD\xCD\xD3\x00\x41\x4C\x66\x00\x5A\x5F" 
		"\x97\x00\xFB\xF9\xF8\x00\x8D\x8E\x92\x00\x51\x62\x98\x00\xE7\xE5\xE7\x00\x96\x98" 
		"\x98\x00\xDE\xDB\xDE\x00\x21\x27\x3F\x00\x4E\x5A\x81\x00\x13\x13\x20\x00\x3B\x43" 
		"\x67\x00\x6B\x6A\x73\x00\x5C\x56\x53\x00\xBA\xBF\xBD\x00\xDD\xDB\xDA\x00\xFD\xFE" 
		"\xFE\x00\x50\x58\x6C\x00\xF6\xF6\xF7\x00\xCD\xCB\xCA\x00\x14\x16\x25\x00\x74\x85" 
		"\xCA\x00\x7A\x81\x83\x00\x73\x69\x6B\x00\x8D\x8D\x93\x00\x00\x00\x00\x00\x00\x00" 
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" 
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" 
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" 
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" 
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" 
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" 
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" 
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" 
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" 
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" 
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" 
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" 
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" 
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" 
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" 
		"\x00\x00\x00\x00\x00\x00\x49\x53\x51\x1D\x9E\x33\x8B\x7F\x65\x37\x09\x0F\x60\x4B" 
		"\x44\x3F\x47\xAC\x89\xAB\x59\x36\xAF\x5E\xAA\xA7\x09\x0E\x63\x85\x28\x79\x6B\xA8" 
		"\x9B\x14\x06\x8A\x62\x35\x66\x9C\x64\x75\x4A\x50\x8E\x04\x1B\x02\x80\x69\x1E\x27" 
		"\x84\x57\x20\x48\x0A\x5D\x68\x4C\x39\x86\x16\x1C\xA0\xA6\x54\x71\x99\x72\x40\x2D" 
		"\x42\x9F\x00\x2F\x96\x4F\x32\x0C\x73\x5A\x2A\x3A\x05\x03\x5B\x5F\x08\x70\x97\x12" 
		"\x8F\x8F\x11\x21\x94\xB0\x43\x8D\x81\x9A\x6F\x2C\x2B\x0D\x8F\x8F\x8F\x34\x8F\x8F" 
		"\x90\x18\x88\x2E\xA4\x7E\x3C\x1F\x77\x07\x34\x8F\x8F\x8F\x8F\x8F\x8F\x8F\x98\x01" 
		"\x45\x4E\x87\x4D\x15\x83\x8F\x34\x8F\x8F\x8F\x8F\x8F\x8F\x5C\x7B\x6E\x6A\x30\x6D" 
		"\x9D\x67\x38\x8F\x8F\x8F\x8F\x8F\x8F\x8F\x25\x7A\x26\x58\x10\xAE\x19\x93\x92\x8F" 
		"\x8F\x8F\x8F\x8F\x8F\x8F\x17\xAD\x0B\x95\x8C\x31\x3D\x29\x41\x8F\x8F\x8F\x8F\x8F" 
		"\x8F\x8F\x52\xA5\x7C\x46\x6C\x78\xA2\x7D\x55\x8F\x8F\x8F\x8F\x8F\x8F\x8F\x8F\x61" 
		"\x74\x13\xA1\x1A\x76\x22\x8F\x34\x8F\x8F\x8F\x8F\x8F\x8F\x8F\x8F\x82\x23\xA3\x24" 
		"\xB1\x3B\x8F\x8F\x8F\x8F\x8F\x8F\x8F\x8F\x34\x8F\xA9\x3E\x56\x55\x91\x8F\x34\x8F" 
		"\x8F\x8F\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" 
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" 
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" 
		"\x00\x00\x00\x00\x00\x00";
	bufferLine("HTTP/1.1 200 OK");
	bufferLine("Content-Type: image/x-icon");
	bufferLine("Content-Length: " + boost::lexical_cast<std::string>(dataLength));
	bufferLine("");
	return write(data, dataLength, true);
}

}  // SeaSocks
