#pragma once

#include "SsoAuthenticator.h"
#include "WebSocket.h"

#include <sys/types.h>

#include <atomic>
#include <cstdint>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace seasocks {

class Connection;
class Logger;
class PageHandler;

class Server {
public:
	Server(std::shared_ptr<Logger> logger);
	~Server();

	void enableSingleSignOn(SsoOptions ssoOptions);

	void setPageHandler(std::shared_ptr<PageHandler> handler);

	void addWebSocketHandler(const char* endpoint, std::shared_ptr<WebSocket::Handler> handler,
			bool allowCrossOriginRequests = false);

	// If we haven't heard anything ever on a connection for this long, kill it.
	// This is possibly caused by bad WebSocket implementation in Chrome.
	void setLameConnectionTimeoutSeconds(int seconds);

	// Sets the maximum number of TCP level keepalives that we can miss before
	// we let the OS consider the connection dead. We configure keepalives every second,
	// so this is also the minimum number of seconds it takes to notice a badly-behaved
	// dead connection, e.g. a laptop going into sleep mode or a hard-crashed machine.
	// A value of 0 disables keep alives, which is the default.
	void setMaxKeepAliveDrops(int maxKeepAliveDrops);

	// Serves static content from the given port on the current thread, until terminate is called.
	// Roughly equivalent to startListening(port); setStaticPath(staticPath); loop();
	// Returns whether exiting was expected.
	bool serve(const char* staticPath, int port);

	// Starts listening on a given interface (in host order) and port.
	// Returns true if all was ok.
	bool startListening(uint32_t ipInHostOrder, int port);

	// Starts listening on a port on all interfaces.
	// Returns true if all was ok.
	bool startListening(int port);

	void setStaticPath(const char* staticPath);

	// Loop (until terminate called from another thread).
	// Returns true if terminate() was used to exit the loop, false if there
	// was an error.
	bool loop();

	// Terminate any loop(). May be called from any thread.
	void terminate();

	void remove(Connection* connection);
	bool subscribeToWriteEvents(Connection* connection);
	bool unsubscribeFromWriteEvents(Connection* connection);

	const std::string& getStaticPath() const { return _staticPath; }
	std::shared_ptr<WebSocket::Handler> getWebSocketHandler(const char* endpoint) const;
	bool isCrossOriginAllowed(const char* endpoint) const;

	std::shared_ptr<PageHandler> getPageHandler() const;

	std::string getStatsDocument() const;

	class Runnable {
	public:
		virtual ~Runnable() {}
		virtual void run() = 0;
	};
	void schedule(std::shared_ptr<Runnable> runnable);

	void checkThread() const;
private:
	bool makeNonBlocking(int fd) const;
	bool configureSocket(int fd) const;
	void handleAccept();
	std::shared_ptr<Runnable> popNextRunnable();
	void processEventQueue();

	void shutdown();

	void checkAndDispatchEpoll();
	void handlePipe();
	enum NewState { KeepOpen, Close };
	NewState handleConnectionEvents(Connection* connection, uint32_t events);

	// Connections, mapped to initial connection time.
	std::map<Connection*, time_t> _connections;
	std::shared_ptr<Logger> _logger;
	int _listenSock;
	int _epollFd;
	int _pipes[2];
	int _maxKeepAliveDrops;
	int _lameConnectionTimeoutSeconds;
	time_t _nextDeadConnectionCheck;

	struct WebSocketHandlerEntry {
		std::shared_ptr<WebSocket::Handler> handler;
		bool allowCrossOrigin;
	};
	typedef std::unordered_map<std::string, WebSocketHandlerEntry> WebSocketHandlerMap;
	WebSocketHandlerMap _webSocketHandlerMap;

	std::shared_ptr<PageHandler> _pageHandler;

	std::mutex _pendingRunnableMutex;
	std::list<std::shared_ptr<Runnable>> _pendingRunnables;
	std::shared_ptr<SsoAuthenticator> _sso;

	pid_t _threadId;

	std::string _staticPath;
    std::atomic<bool> _terminate;
    std::atomic<bool> _expectedTerminate;
};

}  // namespace seasocks
