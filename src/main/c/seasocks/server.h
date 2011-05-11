#ifndef _SEASOCKS_SERVER_H_
#define _SEASOCKS_SERVER_H_

#include "websocket.h"
#include "ssoauthenticator.h"
#include "mutex.h"

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>
#include <list>
#include <map>
#include <string>

namespace SeaSocks {

class Connection;
class Logger;

class Server {
public:
	Server(boost::shared_ptr<Logger> logger);
	~Server();

	void enableSingleSignOn(SsoOptions ssoOptions);
	void addWebSocketHandler(const char* endpoint, boost::shared_ptr<WebSocket::Handler> handler,
			bool allowCrossOriginRequests = false);
	// If we haven't heard anything ever on a connection for this long, kill it.
	// This is possibly caused by bad WebSocket implementation in Chrome.
	void setLameConnectionTimeoutSeconds(int seconds);

	// Serves static content from the given port on the current thread, until terminate is called
	void serve(const char* staticPath, int port);

	void terminate();

	void remove(Connection* connection);
	bool subscribeToWriteEvents(Connection* connection);
	bool unsubscribeFromWriteEvents(Connection* connection);

	const std::string& getStaticPath() const { return _staticPath; }
	boost::shared_ptr<WebSocket::Handler> getWebSocketHandler(const char* endpoint) const;
	bool isCrossOriginAllowed(const char* endpoint) const;

	std::string getStatsDocument() const;

	class Runnable {
	public:
		virtual ~Runnable() {}
		virtual void run() = 0;
	};
	void schedule(boost::shared_ptr<Runnable> runnable);

private:
	bool makeNonBlocking(int fd) const;
	bool configureSocket(int fd) const;
	void handleAccept();
	boost::shared_ptr<Runnable> popNextRunnable();
	void processEventQueue();

	// Connections, mapped to initial connection time.
	std::map<Connection*, time_t> _connections;
	time_t _nextDeadConnectionCheck;
	boost::shared_ptr<Logger> _logger;
	int _listenSock;
	int _epollFd;
	int _pipes[2];
	int _lameConnectionTimeoutSeconds;

	struct Entry {
		boost::shared_ptr<WebSocket::Handler> handler;
		bool allowCrossOrigin;
	};
	typedef boost::unordered_map<std::string, Entry> HandlerMap;
	HandlerMap _handlerMap;

	Mutex _pendingRunnableMutex;
	std::list<boost::shared_ptr<Runnable>> _pendingRunnables;
	boost::shared_ptr<SsoAuthenticator> _sso;

	std::string _staticPath;
    volatile bool _terminate;
};

}  // namespace SeaSocks

#endif  // _SEASOCKS_SERVER_H_
