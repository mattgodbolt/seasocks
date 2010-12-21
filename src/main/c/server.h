#ifndef _SEASOCKS_SERVER_H_
#define _SEASOCKS_SERVER_H_

#include "websocket.h"

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>
#include <string>

namespace SeaSocks {

class Connection;
class Logger;

class Server {
public:
	Server(boost::shared_ptr<Logger> logger);
	~Server();

	void addWebSocketHandler(const char* endpoint, boost::shared_ptr<WebSocket::Handler> handler);

	// Serves static content from the given port on the current thread, forever.
	void serve(const char* staticPath, int port);

	void unsubscribeFromAllEvents(Connection* connection);
	bool subscribeToWriteEvents(Connection* connection);
	bool unsubscribeFromWriteEvents(Connection* connection);

	const char* getStaticPath() const { return _staticPath; }
	boost::shared_ptr<WebSocket::Handler> getWebSocketHandler(const char* endpoint) const;

private:
	void handleAccept();

	boost::shared_ptr<Logger> _logger;
	int _listenSock;
	int _epollFd;

	typedef boost::unordered_map<std::string, boost::shared_ptr<WebSocket::Handler>> HandlerMap;
	HandlerMap _handlerMap;

	const char* _staticPath;
};


}  // namespace SeaSocks

#endif  // _SEASOCKS_SERVER_H_
