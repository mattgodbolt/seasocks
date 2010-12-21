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
	Connection(const char* staticPath);
	~Connection();

	bool accept(int listenSock, int epollFd);
	void close();
	bool write(const void* data, size_t size);
	bool writeLine(const char* line);
	bool handleData(uint32_t event);

private:
	bool handleNewData();
	bool handleHeaders();
	bool handleWebSocketKey3();
	bool handleWebSocket();
	bool handleWebSocketMessage(const char* message);

	// Send an error document. Returns 'true' for convenience in handle*() routines.
	bool sendError(int errorCode, const char* message, const char* document);

	// Send individual errors. Again all return true for convenience.
	bool sendUnsupportedError(const char* reason);
	bool send404(const char* path);
	bool sendBadRequest(const char* reason);
	bool processHeaders(uint8_t* first, uint8_t* last);

	bool sendStaticData(bool keepAlive, const char* authority);

	int _fd;
	int _epollFd;
	bool _closeOnEmpty;
	const char* _staticPath;
	sockaddr_in _address;
	epoll_event _event;
	uint32_t _webSocketKeys[2];
	std::vector<uint8_t> _inBuf;
	std::vector<uint8_t> _outBuf;

	enum State {
		INVALID,
		READING_HEADERS,
		READING_WEBSOCKET_KEY3,
		HANDLING_WEBSOCKET
	};
	State _state;

	Connection(Connection& other) = delete;
	Connection& operator =(Connection& other) = delete;
};

}  // namespace SeaSocks

#endif  // _SEASOCKS_CONNECTION_H_
