#ifndef _SEASOCKS_CONNECTION_H_
#define _SEASOCKS_CONNECTION_H_

#include "websocket.h"

#include <inttypes.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <boost/shared_ptr.hpp>
#include <string>
#include <vector>

namespace SeaSocks {

class Logger;
class Server;

class Connection : public WebSocket {
public:
	Connection(
			boost::shared_ptr<Logger> logger,
			Server* server,
			int fd,
			const sockaddr_in& address);
	~Connection();

	void close();
	bool write(const void* data, size_t size);
	bool writeLine(const char* line);
	bool handleDataReadyForRead();
	bool handleDataReadyForWrite();

	int getFd() const { return _fd; }
	const sockaddr_in& getAddress() const { return _address; }

	// From WebSocket.
	bool respond(const char* webSocketResponse);

private:
	bool checkCloseConditions();

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

	bool sendStaticData(bool keepAlive, const char* requestUri);

	boost::shared_ptr<Logger> _logger;
	Server* _server;
	int _fd;
	bool _closeOnEmpty;
	bool _registeredForWriteEvents;
	sockaddr_in _address;
	uint32_t _webSocketKeys[2];
	std::vector<uint8_t> _inBuf;
	std::vector<uint8_t> _outBuf;
	// Populated only during web socket header parsing.
	std::string _webSockExtraHeaders;
	boost::shared_ptr<WebSocket::Handler> _webSocketHandler;

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
