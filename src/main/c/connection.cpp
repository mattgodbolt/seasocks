#define __USE_GNU  // Get some handy extra functions
#include "connection.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

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

}  // namespace

namespace SeaSocks {

Connection::Connection(const char* staticPath)
	: _fd(-1), _epollFd(-1), _payloadSizeRemaining(0), _state(INVALID), _closeOnEmpty(false), _staticPath(staticPath) {
	assert(staticPath != NULL);
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
	case HANDLING_PAYLOAD:
		return false;
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
			if (_payloadSizeRemaining != 0) {
				_state = HANDLING_PAYLOAD;
			}
			return handleNewData();
		}
	}
	const size_t MAX_HEADERS_SIZE = 65536;
	if (_inBuf.size() > MAX_HEADERS_SIZE) {
		// todo: error doc
		return false;
	}
	return true;
}

void Connection::sendUnsupportedError(const char* reason) {
	// TODO.
	printf("unsup: %s\n", reason);
}

void Connection::send404(const char* path) {
	printf("not found: %s\n", path);
	writeLine("HTTP/1.1 404 not found");
	writeLine("Connection: close");
	writeLine("");
	// TODO
}
void Connection::sendBadRequest(const char* reason) {
	// TODO.
	printf("bad: %s\n", reason);
}

bool Connection::processHeaders(uint8_t* first, uint8_t* last) {
	char* requestLine = extractLine(first, last);
	assert(requestLine != NULL);

	const char* verb = shift(requestLine);
	if (strcmp(verb, "GET") != 0) {
		sendUnsupportedError("We only support GET");
		return false;
	}
	const char* authority = shift(requestLine);
	if (authority == NULL) {
		sendBadRequest("Malformed request line");
		return false;
	}

	const char* httpVersion = shift(requestLine);
	if (httpVersion == NULL) {
		sendBadRequest("Malformed request line");
		return false;
	}
	if (strcmp(httpVersion, "HTTP/1.1") != 0) {
		sendUnsupportedError("Unsupported HTTP version");
		return false;
	}
	if (*requestLine != 0) {
		sendBadRequest("Trailing crap after http version");
		return false;
	}

	printf("authority: %s\n", authority);
	bool keepAlive = false;
	while (first < last) {
		char* colonPos = NULL;
		char* headerLine = extractLine(first, last, &colonPos);
		assert(headerLine != NULL);
		if (colonPos == NULL) {
			sendBadRequest("Malformed header");
			return false;
		}
		*colonPos = 0;
		const char* key = headerLine;
		const char* value = skipWhitespace(colonPos + 1);
		printf("Key: %s || Value: %s\n", key, value);
		if (strcmp(key, "Connection") == 0) {
			if (strcmp(value, "keep-alive") == 0) {
				keepAlive = true;
			}
		}
	}

	char path[1024]; // xx horrible
	strcpy(path, _staticPath);
	strcat(path, authority);
	if (path[strlen(path)-1] == '/') {
		strcat(path, "index.html");
	}
	FILE* f = fopen(path, "rb");
	if (f == NULL) {
		send404(path);
		_closeOnEmpty = true; return true; // AND THE SAME FOR OTHER ERRORS
		return false;
	}
	fseek(f, 0, SEEK_END);
	int fileSize = ftell(f);
	fseek(f, 0, SEEK_SET);
	// todo: check errs
	writeLine("HTTP/1.1 200 OK");
	if (strstr(authority, ".js") != 0) { // xxx
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
