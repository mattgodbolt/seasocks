#include "connection.h"

#include "server.h"
#include "stringutil.h"
#include "logger.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <openssl/md5.h>

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

const struct {
	const char* extension;
	const char* contentType;
} contentTypes[] = {
		{ "html", "text/html" },
		{ "js", "application/javascript" },
		{ "css", "text/css" },
		{ "jpeg", "image/jpeg" },
		{ "png", "image/png" },
};

const char* getContentType(const char* path) {
	const char* extension = strrchr(path, '.');
	if (extension) {
		++extension;
		for (size_t i = 0; i < sizeof(contentTypes) / sizeof(contentTypes[0]); ++i) {
			if (strcasecmp(contentTypes[i].extension, extension) == 0) {
				return contentTypes[i].contentType;
			}
		}
	}
	return "text/html";
}

}  // namespace

namespace SeaSocks {

Connection::Connection(
		boost::shared_ptr<Logger> logger,
		Server* server,
		int fd,
		const sockaddr_in& address,
		const char* staticPath)
	: _logger(logger),
	  _server(server),
	  _fd(fd),
	  _state(READING_HEADERS),
	  _closeOnEmpty(false),
	  _staticPath(staticPath),
	  _registeredForWriteEvents(false),
	  _address(address) {
	assert(staticPath != NULL);
	_webSocketKeys[0] = _webSocketKeys[1] = 0;
}

Connection::~Connection() {
	close();
}

void Connection::close() {
	if (_fd != -1) {
		_server->unsubscribeFromAllEvents(this);
		::close(_fd);
	}
	_fd = -1;
}

bool Connection::write(const void* data, size_t size) {
	assert(!_closeOnEmpty);
	if (size == 0) {
		return true;
	}
	int bytesSent = 0;
	if (_outBuf.empty()) {
		// Attempt fast path, send directly.
		int writeResult = ::write(_fd, data, size);
		if (writeResult == static_cast<int>(size)) {
			// We sent directly.
			return true;
		}
		if (writeResult == -1) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				// Treat this as if zero bytes were written.
				bytesSent = 0;
			} else {
				_logger->error("Unable to write to socket: %s", getLastError());
				return false;
			}
		} else {
			bytesSent = writeResult;
		}
	}
	size_t bytesToBuffer = size - bytesSent;
	size_t endOfBuffer = _outBuf.size();
	_outBuf.resize(endOfBuffer + bytesToBuffer);
	memcpy(&_outBuf[endOfBuffer], reinterpret_cast<const uint8_t*>(data) + bytesSent, bytesToBuffer);
	if (!_registeredForWriteEvents) {
		if (!_server->subscribeToWriteEvents(this)) {
			return false;
		}
		_registeredForWriteEvents = true;
	}
	return true;
}

// TODO: Argh, buffer this up, and send in one go. It's slow otherwise... (wireshark says so anyway)
bool Connection::writeLine(const char* line) {
	static const char crlf[] = { '\r', '\n' };
	if (!write(line, strlen(line))) return false;
	return write(crlf, 2);
}

bool Connection::handleDataReadyForRead() {
	const int READ_SIZE = 16384;
	// TODO: terrible buffer handling.
	size_t curSize = _inBuf.size();
	_inBuf.resize(curSize + READ_SIZE);
	int result = ::read(_fd, &_inBuf[curSize], READ_SIZE);
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
	if (!_outBuf.empty()) {
		int numWritten = ::write(_fd, &_outBuf[0], _outBuf.size());
		if (numWritten == -1) {
			_logger->error("%s : Unable to write to socket: %s", formatAddress(_address), getLastError());
			return false;
		}
		_outBuf.erase(_outBuf.begin(), _outBuf.begin() + numWritten);
	}
	if (_outBuf.empty()) {
		if (!_server->unsubscribeFromWriteEvents(this)) {
			return false;
		}
		_registeredForWriteEvents = false;
	}
	return checkCloseConditions();
}

bool Connection::checkCloseConditions() {
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
			/// XXX horrible
			_inBuf.erase(_inBuf.begin(), _inBuf.begin() + i + 4);
			return handleNewData();
		}
	}
	const size_t MAX_HEADERS_SIZE = 65536;
	if (_inBuf.size() > MAX_HEADERS_SIZE) {
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

	uint8_t digest[MD5_DIGEST_LENGTH];
	MD5(reinterpret_cast<const uint8_t*>(&md5Source), sizeof(md5Source), digest);

	_logger->debug("%s : Attempting websocket upgrade", formatAddress(_address));

	writeLine("HTTP/1.1 101 WebSocket Protocol Handshake");
	writeLine("Upgrade: WebSocket");
	writeLine("Connection: Upgrade");
	write(&_webSockExtraHeaders[0], _webSockExtraHeaders.size());
	writeLine("");

	write(&digest, MD5_DIGEST_LENGTH);

	uint8_t zero = 0;
	write(&zero, 1);
	static const uint8_t testMessage[]  = "console.log('it works!');";
	write(testMessage, sizeof(testMessage) - 1);
	uint8_t effeff = 0xff;
	write(&effeff, 1);

	_state = HANDLING_WEBSOCKET;
	_inBuf.erase(_inBuf.begin(), _inBuf.begin() + 8);

	return true;
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
	const size_t MAX_WEBSOCKET_MESSAGE_SIZE = 16384;
	if (_inBuf.size() > MAX_WEBSOCKET_MESSAGE_SIZE) {
		_logger->error("%s : WebSocket message too long", formatAddress(_address));
		return false;
	}
	return true;
}

bool Connection::handleWebSocketMessage(const char* message) {
	_logger->debug("%s : Got web socket message: '%s'", formatAddress(_address), message);
	return true;
}

bool Connection::sendError(int errorCode, const char* message, const char* document) {
	assert(_state != HANDLING_WEBSOCKET);
	_logger->info("%s : Sending error %d - %s", formatAddress(_address), errorCode, message);
	char buf[1024];
	sprintf(buf, "HTTP/1.1 %d %s", errorCode, message);
	writeLine(buf);
	sprintf(buf, "Content-Length: %d", static_cast<int>(strlen(document)));
	writeLine("Connection: close");
	writeLine("");
	writeLine(document);
	_closeOnEmpty = true;
	return true;
}

bool Connection::sendUnsupportedError(const char* reason) {
	return sendError(501, "Not Implemented", reason);
}

bool Connection::send404(const char* path) {
	return sendError(404, "Not Found", path);
}

bool Connection::sendBadRequest(const char* reason) {
	return sendError(400, "Bad Request", reason);
}

bool Connection::processHeaders(uint8_t* first, uint8_t* last) {
	char* requestLine = extractLine(first, last);
	assert(requestLine != NULL);

	const char* verb = shift(requestLine);
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
		} else if (strcasecmp(key, "Host") == 0) {
			_webSockExtraHeaders += "Sec-WebSocket-Origin: http://";
			_webSockExtraHeaders += value;
			_webSockExtraHeaders += "\r\nSec-WebSocket-Location: ws://";
			_webSockExtraHeaders += value;
			_webSockExtraHeaders += requestUri;
			_webSockExtraHeaders += "\r\n";
		}
	}

	if (haveConnectionUpgrade && haveWebSocketUprade) {
		_logger->debug("%s : Got a websocket with key1=0x%08x, key2=0x%08x", formatAddress(_address), _webSocketKeys[0], _webSocketKeys[1]);
		_state = READING_WEBSOCKET_KEY3;
		return true;
	} else {
		return sendStaticData(keepAlive, requestUri);
	}
}

bool Connection::sendStaticData(bool keepAlive, const char* requestUri) {
	char path[1024]; // xx horrible
	strcpy(path, _staticPath);
	strcat(path, requestUri);
	// Trim any trailing queries.
	char* query = strchr(path, '?');
	if (query != NULL) {
		*query = 0;
	}
	if (path[strlen(path)-1] == '/') {
		strcat(path, "index.html");
	}
	FILE* f = fopen(path, "rb");
	if (f == NULL) {
		return send404(path);
	}
	if (fseek(f, 0, SEEK_END) == -1) {
		fclose(f);
		return send404(path);
	}
	int fileSize = ftell(f);
	if (fileSize < 0) {
		fclose(f);
		return send404(path);
	}
	if (fseek(f, 0, SEEK_SET) == -1) {
		fclose(f);
		return send404(path);
	}
	writeLine("HTTP/1.1 200 OK");
	char buf[16384];
	sprintf(buf, "Content-Type: %s", getContentType(path));
	writeLine(buf);
	if (keepAlive) {
		writeLine("Connection: keep-alive");
	} else {
		writeLine("Connection: close");
	}
	char contentLength[128];
	sprintf(contentLength, "Content-Length: %d", fileSize);
	writeLine(contentLength);
	writeLine("");

	int total = 0;
	for (;;) {
		int bytesRead = fread(buf, 1, sizeof(buf), f);
		total += bytesRead;
		if (bytesRead == -1) {
			_logger->error("%s : Error reading file: ", formatAddress(_address), getLastError());
			fclose(f);
			// We can't send an error document as we've sent the header.
			return false;
		}
		if (bytesRead == 0) {
			break;
		}
		if (!write(buf, bytesRead)) {
			fclose(f);
			return false;
		}
	}
	fclose(f);
	if (!keepAlive) {
		_logger->debug("%s : Closing on empty", formatAddress(_address));
		_closeOnEmpty = true;
	}
	return true;
}

}  // SeaSocks
