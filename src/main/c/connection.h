#ifndef _SEASOCKS_CONNECTION_H_
#define _SEASOCKS_CONNECTION_H_

#include <inttypes.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#include <vector>

namespace SeaSocks {

class Connection {
public:
	Connection();
	~Connection();

	bool accept(int listenSock, int epollFd);
	void close();
	bool write(const void* data, size_t size);
	bool writeLine(const char* line);
	bool handleData(uint32_t event);

private:
	bool handleNewData();
	bool handleHeaders();

	void sendUnsupportedError(const char* reason);
	void send404(const char* path);
	void sendBadRequest(const char* reason);
	bool processHeaders(uint8_t* first, uint8_t* last);

	int _fd;
	int _epollFd;
	int _payloadSizeRemaining;
	bool _closeOnEmpty;
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

}  // namespace SeaSocks

#endif  // _SEASOCKS_CONNECTION_H_
