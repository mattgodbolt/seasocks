#include <stdio.h>

#include <errno.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#include <memory>
#include <vector>

class Connection {
public:
	Connection() : _fd(-1), _epollFd(-1) {}
	~Connection() {
		close();
	}

	bool accept(int listenSock, int epollFd) {
		socklen_t addrLen = sizeof(_address);
		_fd = ::accept(listenSock, reinterpret_cast<sockaddr*>(&_address), &addrLen);
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

	bool handleData(uint32_t event) {
		if (event & EPOLLIN) {
			uint8_t buf[1024];
			int result = read(_fd, buf, sizeof(buf));
			if (result == -1) {
				perror("read");
				return false;
			}
			if (result == 0) {
				printf("Remote end closed\n");
				return false;
			}
			printf("Got %d bytes of data\n", result);
			if (!write(buf, result)) {
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
	int _fd;
	int _epollFd;
	sockaddr_in _address;
	epoll_event _event;
	std::vector<uint8_t> _outBuf;

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

