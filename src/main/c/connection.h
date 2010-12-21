#ifndef _SEASOCKS_CONNECTION_H_
#define _SEASOCKS_CONNECTION_H_

#include <inttypes.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <boost/shared_ptr.hpp>
#include <string>
#include <vector>

namespace SeaSocks {

class Logger;
class Server;

class Connection {
public:
	Connection(
			boost::shared_ptr<Logger> logger,
			Server* server,
			int fd,
			const sockaddr_in& address,
			const char* staticPath);
	~Connection();

	void close();
	bool write(const void* data, size_t size);
	bool writeLine(const char* line);
	bool handleDataReadyForRead();
	bool handleDataReadyForWrite();

	int getFd() const { return _fd; }
	const sockaddr_in& getAddress() const { return _address; }

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

	bool sendStaticData(bool keepAlive, const char* authority);

	boost::shared_ptr<Logger> _logger;
	Server* _server;
	int _fd;
	bool _closeOnEmpty;
	const char* _staticPath;
	bool _registeredForWriteEvents;
	sockaddr_in _address;
	uint32_t _webSocketKeys[2];
	std::vector<uint8_t> _inBuf;
	std::vector<uint8_t> _outBuf;
	// Populated only during web socket header parsing.
	std::string _webSockExtraHeaders;

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
