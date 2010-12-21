#define __USE_GNU  // Get some handy extra functions
#include "connection.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <openssl/md5.h>

namespace {

static char* skipWhitespace(char* str) {
	while (isspace(*str)) ++str;
	return str;
}

static char* skipNonWhitespace(char* str) {
	while (*str && !isspace(*str)) {
		++str;
	}
	return str;
}

static char* shift(char*& str) {
	if (str == NULL) {
		return NULL;
	}
	char* startOfWord = skipWhitespace(str);
	if (*startOfWord == 0) {
		str = startOfWord;
		return NULL;
	}
	char* endOfWord = skipNonWhitespace(startOfWord);
	if (*endOfWord != 0) {
		*endOfWord++ = 0;
	}
	str = endOfWord;
	return startOfWord;
}

char* extractLine(uint8_t*& first, uint8_t* last, char** colon = NULL) {
	uint8_t* ptr = first;
	for (uint8_t* ptr = first; ptr < last - 1; ++ptr) {
		if (ptr[0] == '\r' && ptr[1] == '\n') {
			ptr[0] = 0;
			uint8_t* result = first;
			first = ptr + 2;
			return reinterpret_cast<char*>(result);
		}
		if (colon && ptr[0] == ':' && *colon == NULL) {
			*colon = reinterpret_cast<char*>(ptr);
		}
	}
	return NULL;
}

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
	printf("Read number %u with numspaces=%d\n", keyNumber, numSpaces);
	return numSpaces > 0 ? keyNumber / numSpaces : 0;
}

}  // namespace

namespace SeaSocks {

Connection::Connection(const char* staticPath)
	: _fd(-1), _epollFd(-1), _state(INVALID), _closeOnEmpty(false), _staticPath(staticPath) {
	assert(staticPath != NULL);
	_webSocketKeys[0] = _webSocketKeys[1] = 0;
}

Connection::~Connection() {
	close();
}

bool Connection::accept(int listenSock, int epollFd) {
	socklen_t addrLen = sizeof(_address);
	_fd = ::accept4(listenSock,
			reinterpret_cast<sockaddr*>(&_address),
			&addrLen,
			SOCK_NONBLOCK | SOCK_CLOEXEC);
	if (_fd == -1) {
		perror("accept");
		return false;
	}
	struct linger linger;
	linger.l_linger = 500; // 5 seconds
	linger.l_onoff = true;
	if (setsockopt(_fd, SOL_SOCKET, SO_LINGER, &linger, sizeof(linger)) == -1) {
		perror("setsockopt");
		return false;
	}
	int yesPlease = 1;
	if (setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &yesPlease, sizeof(yesPlease)) == -1) {
		perror("setsockopt");
		return false;
	}
	printf("accept on %d\n", _fd);
	_event.events = EPOLLIN;// | EPOLLOUT;
	_event.data.ptr = this;
	if (epoll_ctl(epollFd, EPOLL_CTL_ADD, _fd, &_event) == -1) {
		perror("epoll_ctl");
		// TODO: error;
		return false;
	}
	_epollFd = epollFd;
	_state = READING_HEADERS;
	return true;
}

void Connection::close() {
	if (_fd != -1) {
		if (_epollFd != -1) {
			epoll_ctl(_epollFd, EPOLL_CTL_DEL, _fd, &_event);
		}
		::close(_fd);
	}
	_fd = -1;
}

bool Connection::write(const void* data, size_t size) {
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
				perror("write");
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
	if ((_event.events & EPOLLOUT) == 0) {
		_event.events |= EPOLLOUT;
		if (epoll_ctl(_epollFd, EPOLL_CTL_MOD, _fd, &_event) == -1) {
			perror("epoll_ctl");
			return false;
		}
	}
	return true;
}

// TODO: Argh, buffer this up, and send in one go. It's slow otherwise... (wireshark says so anyway)
bool Connection::writeLine(const char* line) {
	static const char crlf[] = { '\r', '\n' };
	if (!write(line, strlen(line))) return false;
	return write(crlf, 2);
}

bool Connection::handleData(uint32_t event) {
	const int READ_SIZE = 16384;
	if (event & EPOLLIN) {
		// TODO: terrible buffer handling.
		auto curSize = _inBuf.size();
		_inBuf.resize(curSize + READ_SIZE);
		int result = ::read(_fd, &_inBuf[curSize], READ_SIZE);
		if (result == -1) {
			perror("read");
			return false;
		}
		if (result == 0) {
			printf("Remote end closed\n");
			return false;
		}
		_inBuf.resize(curSize + result);
		if (!handleNewData()) {
			return false;
		}
	}
	if (event & EPOLLOUT) {
		if (!_outBuf.empty()) {
			int numWritten = ::write(_fd, &_outBuf[0], _outBuf.size());
			if (numWritten == -1) {
				perror("write");
				return false;
			}
			_outBuf.erase(_outBuf.begin(), _outBuf.begin() + numWritten);
		}
		if (_outBuf.empty()) {
			_event.events &= ~EPOLLOUT;
			if (epoll_ctl(_epollFd, EPOLL_CTL_MOD, _fd, &_event) == -1) {
				perror("epoll_ctl");
				return false;
			}
		}
	}
	if (_closeOnEmpty && _outBuf.empty()) {
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

	printf("Attempting websocket upgrade\n");

	writeLine("HTTP/1.1 101 WebSocket Protocol Handshake");
	writeLine("Upgrade: WebSocket");
	writeLine("Connection: Upgrade");
	writeLine("Sec-WebSocket-Origin: http://localhost:9090");  // staggering hack
	writeLine("Sec-WebSocket-Location: ws://localhost:9090/dump");  // staggering hack
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
			printf("Error in WebSocket input stream (got 0x%02x)\n", _inBuf[messageStart]);
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
		printf("WebSocket message too long\n");
		return false;
	}
	return true;
}

bool Connection::handleWebSocketMessage(const char* message) {
	printf("Got web socket message: %s\n", message);
	return true;
}

bool Connection::sendError(int errorCode, const char* message, const char* document) {
	printf("Error: %d - %s\n", errorCode, message);
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
	// TODO: it's not the authority...
	const char* authority = shift(requestLine);
	if (authority == NULL) {
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

	printf("authority: %s\n", authority);
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
		printf("Key: %s || Value: %s\n", key, value);
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
		}
	}

	if (haveConnectionUpgrade && haveWebSocketUprade) {
		printf("Got a websocket with key1=0x%08x, key2=0x%08x\n", _webSocketKeys[0], _webSocketKeys[1]);
		_state = READING_WEBSOCKET_KEY3;
		return true;
	} else {
		return sendStaticData(keepAlive, authority);
	}
}

bool Connection::sendStaticData(bool keepAlive, const char* authority) {
	char path[1024]; // xx horrible
	strcpy(path, _staticPath);
	strcat(path, authority);
	if (path[strlen(path)-1] == '/') {
		strcat(path, "index.html");
	}
	FILE* f = fopen(path, "rb");
	if (f == NULL) {
		return send404(path);
	}
	fseek(f, 0, SEEK_END);
	int fileSize = ftell(f);
	fseek(f, 0, SEEK_SET);
	// todo: check errs
	writeLine("HTTP/1.1 200 OK");
	if (strstr(path, ".js") != 0) { // xxx
		writeLine("Content-Type: text/javascript");
	} else {
		writeLine("Content-Type: text/html");
	}
	if (keepAlive) {
		writeLine("Connection: keep-alive");
	} else {
		writeLine("Connection: close");
	}
	char contentLength[128];
	sprintf(contentLength, "Content-Length: %d", fileSize);
	writeLine(contentLength);
	writeLine("");
	char buf[16384];
	int total = 0;
	for (;;) {
		int bytesRead = fread(buf, 1, sizeof(buf), f);
		total += bytesRead;
		if (bytesRead <= 0) break; // TODO error
		write(buf, bytesRead); // todo check errs
	}
	fclose(f);
	if (!keepAlive) {
		_closeOnEmpty = true;
	}
	return true;
}

}  // SeaSocks
