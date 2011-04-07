#ifndef _SEASOCKS_SERVER_H_
#define _SEASOCKS_SERVER_H_

#include "websocket.h"
#include "ssoauthenticator.h"
#include "mutex.h"

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>
#include <list>
#include <string>

namespace SeaSocks {

class Connection;
class Logger;

class Server {
public:
	Server(boost::shared_ptr<Logger> logger);
	~Server();

	void addWebSocketHandler(const char* endpoint, boost::shared_ptr<WebSocket::Handler> handler);

	// Serves static content from the given port on the current thread, until terminate is called
	void serve(const char* staticPath, int port);

    void terminate();

	void unsubscribeFromAllEvents(Connection* connection);
	bool subscribeToWriteEvents(Connection* connection);
	bool unsubscribeFromWriteEvents(Connection* connection);

	const char* getStaticPath() const { return _staticPath; }
	boost::shared_ptr<WebSocket::Handler> getWebSocketHandler(const char* endpoint) const;

	class Runnable {
	public:
		virtual ~Runnable() {}
		virtual void run() = 0;
	};
	void schedule(boost::shared_ptr<Runnable> runnable);

private:
	bool configureSocket(int fd) const;
	void handleAccept();
	boost::shared_ptr<Runnable> popNextRunnable();

	boost::shared_ptr<Logger> _logger;
	int _listenSock;
	int _epollFd;
	int _pipes[2];

	typedef boost::unordered_map<std::string, boost::shared_ptr<WebSocket::Handler>> HandlerMap;
	HandlerMap _handlerMap;

	Mutex _pendingRunnableMutex;
	std::list<boost::shared_ptr<Runnable>> _pendingRunnables;
	boost::shared_ptr<SsoAuthenticator> _sso;

	const char* _staticPath;
	volatile bool _terminate;
};


}  // namespace SeaSocks

#endif  // _SEASOCKS_SERVER_H_
