#define __USE_GNU  // Get some handy extra functions
#include <stdio.h>

#include <errno.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#include <string>
#include <string.h>
#include <assert.h>

#include <memory>
#include <vector>

class Connection {
public:
	Connection() : _fd(-1), _epollFd(-1), _payloadSizeRemaining(0), _state(INVALID) {}
	~Connection() {
		close();
	}

	bool accept(int listenSock, int epollFd) {
		socklen_t addrLen = sizeof(_address);
		_fd = ::accept4(listenSock,
				reinterpret_cast<sockaddr*>(&_address),
				&addrLen,
				SOCK_NONBLOCK | SOCK_CLOEXEC);
		if (_fd == -1) {
			perror("accept");
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

	void close() {
		if (_fd != -1) {
			if (_epollFd != -1) {
				epoll_ctl(_epollFd, EPOLL_CTL_DEL, _fd, &_event);
			}
			::close(_fd);
		}
		_fd = -1;
	}

	bool write(const void* data, size_t size) {
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
					writeResult = 0;
				} else {
					perror("write");
					return false;
				}
			}
			size_t bytesToBuffer = size - writeResult;
			size_t endOfBuffer = _outBuf.size();
			_outBuf.resize(endOfBuffer + bytesToBuffer);
			memcpy(&_outBuf[endOfBuffer], reinterpret_cast<const uint8_t*>(data) + writeResult, bytesToBuffer);
			if ((_event.events & EPOLLOUT) == 0) {
				_event.events |= EPOLLOUT;
				if (epoll_ctl(_epollFd, EPOLL_CTL_MOD, _fd, &_event) == -1) {
					perror("epoll_ctl");
					return false;
				}
			}
		}
		return true;
	}

	bool writeLine(const char* line) {
		static const char crlf[] = { '\r', '\n' };
		if (!write(line, strlen(line))) return false;
		return write(crlf, 2);
	}

	bool handleData(uint32_t event) {
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
		return true;
	}

private:

	bool handleNewData() {
		switch (_state) {
		case READING_HEADERS:
			return handleHeaders();
		case HANDLING_PAYLOAD:
			return false;
		default:
			return false;
		}
	}

	bool handleHeaders() {
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

	void sendUnsupportedError(const char* reason) {
		// TODO.
		printf("unsup: %s\n", reason);
	}

	void send404(const char* path) {
		printf("not found: %s\n", path);
		// todo
	}
	void sendBadRequest(const char* reason) {
		// TODO.
		printf("bad: %s\n", reason);
	}

	bool processHeaders(uint8_t* first, uint8_t* last) {
		char* requestLine = extractLine(first, last);
		assert(requestLine != NULL);
		printf("Req line: %s\n", requestLine);

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
		}

		char path[1024]; // xx horrible
		strcpy(path, "./src/web");
		strcat(path, authority);
		FILE* f = fopen(path, "rb");
		if (f == NULL) {
			send404(path);
			return false;
		}
		// todo: check errs
		writeLine("HTTP/1.1 200 OK");
		writeLine("Content-Type: text/html");
		writeLine("Connection: close");
		writeLine("");
		char buf[16384];
		for (;;) {
			int bytesRead = fread(buf, 1, sizeof(buf), f);
			if (bytesRead <= 0) break; // TODO error
			write(buf, bytesRead); // todo check errs
		}
		return false;
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

	int _fd;
	int _epollFd;
	int _payloadSizeRemaining;
	sockaddr_in _address;
	epoll_event _event;
	std::vector<uint8_t> _inBuf;
	std::vector<uint8_t> _outBuf;

	enum State {
		INVALID,
		READING_HEADERS,
		HANDLING_PAYLOAD,
	};
	State _state;

	Connection(Connection& other) = delete;
	Connection& operator =(Connection& other) = delete;
};

class Server {
public:
	Server()
		: _listenSock(-1), _epollFd(-1) {

	}
	~Server() {
		if (_listenSock != -1) {
			close(_listenSock);
		}
		if (_epollFd != -1) {
			close(_epollFd);
		}
	}

	void serve(int port) {
		_listenSock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
		if (_listenSock == -1) {
			perror("socket");
			return;
		}
		sockaddr_in sock;
		memset(&sock, 0, sizeof(sock));
		sock.sin_port = htons(port);
		sock.sin_addr.s_addr = INADDR_ANY;
		sock.sin_family = AF_INET;
		if (bind(_listenSock, reinterpret_cast<const sockaddr*>(&sock), sizeof(sock)) == -1) {
			perror("bind");
			return;
		}
		if (listen(_listenSock, 5) == -1) {
			perror("listen");
			return;
		}
		int yesPlease = 1;
		if (setsockopt(_listenSock, SOL_SOCKET, SO_REUSEADDR, &yesPlease, sizeof(yesPlease)) == -1) {
			perror("setsockopt");
			return;
		}

		_epollFd = epoll_create1(EPOLL_CLOEXEC);
		if (_epollFd == -1) {
			perror("epoll_create");
			return;
		}

		epoll_event event;
		event.events = EPOLLIN;
		event.data.ptr = this;
		if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, _listenSock, &event) == -1) {
			perror("epoll_ctl");
			return;
		}

		const int maxEvents = 20;
		epoll_event events[maxEvents];

		for (;;) {
			int numEvents = epoll_wait(_epollFd, events, maxEvents, -1);
			if (numEvents == -1) {
				perror("epoll_wait");
				return;
			}
			for (int i = 0; i < numEvents; ++i) {
				if (events[i].data.ptr == this) {
					handleAccept();
				} else {
					auto connection = reinterpret_cast<Connection*>(events[i].data.ptr);
					if (!connection->handleData(events[i].events)) {
						connection->close();
						delete connection;
					}
				}
			}
		}
	}

	void handleAccept() {
		std::auto_ptr<Connection> newConnection(new Connection());
		if (!newConnection->accept(_listenSock, _epollFd)) {
			perror("accept");
			// TODO: error
			return;
		}
		newConnection.release();
	}

private:
	int _listenSock;
	int _epollFd;
};

int main(int argc, const char* argv[]) {
	Server serve;
	serve.serve(9090);
	return 1;
}

