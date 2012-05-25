#include "seasocks/server.h"

#include "seasocks/connection.h"
#include "seasocks/stringutil.h"
#include "seasocks/logger.h"
#include "seasocks/util/Json.h"

#include "internal/LogStream.h"

#include <string.h>

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <stdexcept>
#include <unistd.h>

#include <memory>

#include <sys/syscall.h>

namespace {

struct EventBits {
	uint32_t bits;
	explicit EventBits(uint32_t bits) : bits(bits) {}
};

std::ostream& operator <<(std::ostream& o, const EventBits& b) {
	uint32_t bits = b.bits;
#define DO_BIT(NAME) \
	do { if (bits & (NAME)) { if (bits != b.bits) {o << ", "; } o << #NAME; bits &= ~(NAME); } } while (0)
    DO_BIT(EPOLLIN);
    DO_BIT(EPOLLPRI);
    DO_BIT(EPOLLOUT);
    DO_BIT(EPOLLRDNORM);
    DO_BIT(EPOLLRDBAND);
    DO_BIT(EPOLLWRNORM);
    DO_BIT(EPOLLWRBAND);
    DO_BIT(EPOLLMSG);
    DO_BIT(EPOLLERR);
    DO_BIT(EPOLLHUP);
#ifdef EPOLLRDHUP
    DO_BIT(EPOLLRDHUP);
#endif
    DO_BIT(EPOLLONESHOT);
    DO_BIT(EPOLLET);
#undef DO_BIT
	return o;
}

const int EpollTimeoutMillis = 500;  // Twice a second is ample.
const int DefaultLameConnectionTimeoutSeconds = 10;
int gettid() {
	return syscall(SYS_gettid);
}

}

namespace SeaSocks {

Server::Server(std::shared_ptr<Logger> logger)
	: _logger(logger), _listenSock(-1), _epollFd(-1), _maxKeepAliveDrops(0),
	  _lameConnectionTimeoutSeconds(DefaultLameConnectionTimeoutSeconds),
	  _nextDeadConnectionCheck(0), _threadId(0),  _terminate(false) {
	_pipes[0] = _pipes[1] = -1;
	_sso = std::shared_ptr<SsoAuthenticator>();

	_epollFd = epoll_create(10);
	if (_epollFd == -1) {
		LS_ERROR(_logger, "Unable to create epoll: " << getLastError());
		return;
	}

	/* TASK: once RH5 is dead and gone, use: _wakeFd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC); instead. It's lower overhead than this. */
	if (pipe(_pipes) != 0) {
		LS_ERROR(_logger, "Unable to create event signal pipe: " << getLastError());
		return;
	}
	if (!makeNonBlocking(_pipes[0]) || !makeNonBlocking(_pipes[1])) {
		LS_ERROR(_logger, "Unable to create event signal pipe: " << getLastError());
		return;
	}

	epoll_event eventWake = { EPOLLIN, { &_pipes[0] } };
	if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, _pipes[0], &eventWake) == -1) {
		LS_ERROR(_logger, "Unable to add wake socket to epoll: " << getLastError());
		return;
	}
}

void Server::enableSingleSignOn(SsoOptions ssoOptions) {
	_sso.reset(new SsoAuthenticator(ssoOptions));
}

Server::~Server() {
	LS_INFO(_logger, "Server destruction");
	shutdown();
	// Only shut the pipes and epoll at the very end so any scheduled work
	if (_pipes[0] != -1) {
		close(_pipes[0]);
	}
	if (_pipes[1] != -1) {
		close(_pipes[1]);
	}
	if (_epollFd != -1) {
		close(_epollFd);
	}
}

void Server::shutdown() {
	// Stop listening to any further incoming connections.
	if (_listenSock != -1) {
		close(_listenSock);
	}
	// Disconnect and close any current connections.
	while (!_connections.empty()) {
		// Deleting the connection closes it and removes it from 'this'.
		Connection* toBeClosed = _connections.begin()->first;
		toBeClosed->setLinger();
		delete toBeClosed;
	}
}

bool Server::makeNonBlocking(int fd) const {
	int yesPlease = 1;
	if (ioctl(fd, FIONBIO, &yesPlease) != 0) {
		LS_ERROR(_logger, "Unable to make FD non-blocking: " << getLastError());
		return false;
	}
	return true;
}

bool Server::configureSocket(int fd) const {
	if (!makeNonBlocking(fd)) {
		return false;
	}
	const int yesPlease = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yesPlease, sizeof(yesPlease)) == -1) {
		LS_ERROR(_logger, "Unable to set reuse socket option: " << getLastError());
		return false;
	}
	if (_maxKeepAliveDrops > 0) {
		if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &yesPlease, sizeof(yesPlease)) == -1) {
			LS_ERROR(_logger, "Unable to enable keepalive: " << getLastError());
			return false;
		}
		const int oneSecond = 1;
		if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &oneSecond, sizeof(oneSecond)) == -1) {
			LS_ERROR(_logger, "Unable to set idle probe: " << getLastError());
			return false;
		}
		if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &oneSecond, sizeof(oneSecond)) == -1) {
			LS_ERROR(_logger, "Unable to set idle interval: " << getLastError());
			return false;
		}
		if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &_maxKeepAliveDrops, sizeof(_maxKeepAliveDrops)) == -1) {
			LS_ERROR(_logger, "Unable to set keep alive count: " << getLastError());
			return false;
		}
	}
	return true;
}

void Server::terminate() {
    _terminate = true;
    uint64_t one = 1;
	if (_pipes[1] != -1 && ::write(_pipes[1], &one, sizeof(one)) == -1) {
		LS_ERROR(_logger, "Unable to post a wake event: " << getLastError());
	}
}

bool Server::startListening(int port) {
	_listenSock = socket(AF_INET, SOCK_STREAM, 0);
	if (_listenSock == -1) {
		LS_ERROR(_logger, "Unable to create listen socket: " << getLastError());
		return false;
	}
	if (!configureSocket(_listenSock)) {
		return false;
	}
	sockaddr_in sock;
	memset(&sock, 0, sizeof(sock));
	sock.sin_port = htons(port);
	sock.sin_addr.s_addr = INADDR_ANY;
	sock.sin_family = AF_INET;
	if (bind(_listenSock, reinterpret_cast<const sockaddr*>(&sock), sizeof(sock)) == -1) {
		LS_ERROR(_logger, "Unable to bind socket: " << getLastError());
		return false;
	}
	if (listen(_listenSock, 5) == -1) {
		LS_ERROR(_logger, "Unable to listen on socket: " << getLastError());
		return false;
	}
	epoll_event event = { EPOLLIN, { this } };
	if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, _listenSock, &event) == -1) {
		LS_ERROR(_logger, "Unable to add listen socket to epoll: " << getLastError());
		return false;
	}

	return true;
}

void Server::handlePipe() {
	uint64_t dummy;
	while (::read(_pipes[0], &dummy, sizeof(dummy)) != -1) {
		// Spin, draining the pipe until it returns EWOULDBLOCK or similar.
	}
	if (errno != EAGAIN || errno != EWOULDBLOCK) {
		LS_ERROR(_logger, "Error from wakeFd read: " << getLastError());
        _terminate = true;
	}
	// It's a "wake up" event; this will just cause the epoll loop to wake up.
}

Server::NewState Server::handleConnectionEvents(Connection* connection, uint32_t events) {
	if (events & ~(EPOLLIN|EPOLLOUT|EPOLLHUP|EPOLLERR)) {
		LS_WARNING(_logger, "Got unhandled epoll event (" << EventBits(events) << ") on connection: "
				<< formatAddress(connection->getRemoteAddress()));
		return Close;
	} else if (events & EPOLLERR) {
		LS_INFO(_logger, "Error on socket (" << EventBits(events) << "): "
				<< formatAddress(connection->getRemoteAddress()));
		return Close;
	} else if (events & EPOLLHUP) {
		LS_DEBUG(_logger, "Graceful hang-up (" << EventBits(events) << ") of socket: "
				<< formatAddress(connection->getRemoteAddress()));
		return Close;
	} else {
		if (events & EPOLLOUT) {
			connection->handleDataReadyForWrite();
		}
		if (events & EPOLLIN) {
			connection->handleDataReadyForRead();
		}
	}
	return KeepOpen;
}

void Server::checkAndDispatchEpoll() {
	const int maxEvents = 256;
	epoll_event events[maxEvents];

	std::list<Connection*> toBeDeleted;
	int numEvents = epoll_wait(_epollFd, events, maxEvents, EpollTimeoutMillis);
	if (numEvents == -1) {
		if (errno != EINTR) {
			LS_ERROR(_logger, "Error from epoll_wait: " << getLastError());
		}
		return;
	}
	if (numEvents == maxEvents) {
		static time_t lastWarnTime = 0;
		time_t now = time(NULL);
		if (now - lastWarnTime >= 60) {
			LS_WARNING(_logger, "Full event queue; may start starving connections. "
					   "Will warn at most once a minute");
			lastWarnTime = now;
		}
	}
	for (int i = 0; i < numEvents; ++i) {
		if (events[i].data.ptr == this) {
			if (events[i].events & ~EPOLLIN) {
				LS_SEVERE(_logger, "Got unexpected event on listening socket ("
						<< EventBits(events[i].events) << ") - terminating");
				_terminate = true;
				break;
			}
			handleAccept();
		} else if (events[i].data.ptr == &_pipes[0]) {
			if (events[i].events & ~EPOLLIN) {
				LS_SEVERE(_logger, "Got unexpected event on management pipe ("
						<< EventBits(events[i].events) << ") - terminating");
				_terminate = true;
				break;
			}
			handlePipe();
		} else {
			auto connection = reinterpret_cast<Connection*>(events[i].data.ptr);
			if (handleConnectionEvents(connection, events[i].events) == Close) {
				toBeDeleted.push_back(connection);
			}
		}
	}
	// The connections are all deleted at the end so we've processed any other subject's
	// closes etc before we call onDisconnect().
	for (auto it = toBeDeleted.begin(); it != toBeDeleted.end(); ++it) {
		auto connection = *it;
		if (_connections.find(connection) == _connections.end()) {
			LS_SEVERE(_logger, "Attempt to delete connection we didn't know about: " << (void*)connection
					<< formatAddress(connection->getRemoteAddress()));
			_terminate = true;
			break;
		}
		LS_DEBUG(_logger, "Deleting connection: " << formatAddress(connection->getRemoteAddress()));
		delete connection;
	}
}

void Server::serve(const char* staticPath, int port) {
	if (_epollFd == -1 || _pipes[0] == -1 || _pipes[1] == -1) {
		LS_ERROR(_logger, "Unable to serve, did not initialize properly.");
		return;
	}
	// Stash away "the" server thread id and the path we're serving from.
	_threadId = gettid();
	_staticPath = staticPath;

	if (!startListening(port)) {
		return;
	}

	char buf[1024];
	::gethostname(buf, sizeof(buf));
	LS_INFO(_logger, "Listening on http://" << buf << ":" << port << "/");

	while (!_terminate) {
		// Always process events first to catch start up events.
		processEventQueue();
		checkAndDispatchEpoll();
	}
	// Reasonable effort to ensure anything enqueued during terminate has a chance to run.
	processEventQueue();
	LS_INFO(_logger, "Server terminating");
	shutdown();
}

void Server::processEventQueue() {
	for (;;) {
		std::shared_ptr<Runnable> runnable = popNextRunnable();
		if (!runnable) break;
		runnable->run();
	}
	time_t now = time(NULL);
	if (now >= _nextDeadConnectionCheck) {
		std::list<Connection*> toRemove;
		for (auto it = _connections.cbegin(); it != _connections.cend(); ++it) {
			time_t numSecondsSinceConnection = now - it->second;
			auto connection = it->first;
			if (connection->bytesReceived() == 0 && numSecondsSinceConnection >= _lameConnectionTimeoutSeconds) {
				LS_INFO(_logger, formatAddress(connection->getRemoteAddress())
						<< " : Killing lame connection - no bytes received after " << numSecondsSinceConnection << "s");
				toRemove.push_back(connection);
			}
		}
		for (auto it = toRemove.begin(); it != toRemove.end(); ++it) {
			delete *it;
		}
	}
}

void Server::handleAccept() {
	sockaddr_in address;
	socklen_t addrLen = sizeof(address);
	int fd = ::accept(_listenSock,
			reinterpret_cast<sockaddr*>(&address),
			&addrLen);
	if (fd == -1) {
		LS_ERROR(_logger, "Unable to accept: " << getLastError());
		return;
	}
	if (!configureSocket(fd)) {
		::close(fd);
		return;
	}
	LS_INFO(_logger, formatAddress(address) << " : Accepted on descriptor " << fd);
	Connection* newConnection = new Connection(_logger, this, fd, address, _sso);
	epoll_event event = { EPOLLIN, { newConnection } };
	if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, fd, &event) == -1) {
		LS_ERROR(_logger, "Unable to add socket to epoll: " << getLastError());
		delete newConnection;
		::close(fd);
		return;
	}
	_connections.insert(std::make_pair(newConnection, time(NULL)));
}

void Server::remove(Connection* connection) {
	checkThread();
	epoll_event event = { 0, { connection } };
	if (epoll_ctl(_epollFd, EPOLL_CTL_DEL, connection->getFd(), &event) == -1) {
		LS_ERROR(_logger, "Unable to remove from epoll: " << getLastError());
	}
	_connections.erase(connection);
}

bool Server::subscribeToWriteEvents(Connection* connection) {
	epoll_event event = { EPOLLIN | EPOLLOUT, { connection } };
	if (epoll_ctl(_epollFd, EPOLL_CTL_MOD, connection->getFd(), &event) == -1) {
		LS_ERROR(_logger, "Unable to subscribe to write events: " << getLastError());
		return false;
	}
	return true;
}

bool Server::unsubscribeFromWriteEvents(Connection* connection) {
	epoll_event event = { EPOLLIN, { connection } };
	if (epoll_ctl(_epollFd, EPOLL_CTL_MOD, connection->getFd(), &event) == -1) {
		LS_ERROR(_logger, "Unable to unsubscribe from write events: " << getLastError());
		return false;
	}
	return true;
}

void Server::addWebSocketHandler(const char* endpoint, std::shared_ptr<WebSocket::Handler> handler,
		bool allowCrossOriginRequests) {
	_webSocketHandlerMap[endpoint] = { handler, allowCrossOriginRequests };
}

void Server::setPageHandler(std::shared_ptr<PageHandler> handler) {
	_pageHandler = handler;
}

bool Server::isCrossOriginAllowed(const char* endpoint) const {
	auto splits = split(endpoint, '?');
	auto iter = _webSocketHandlerMap.find(splits[0]);
	if (iter == _webSocketHandlerMap.end()) {
		return false;
	}
	return iter->second.allowCrossOrigin;
}

std::shared_ptr<WebSocket::Handler> Server::getWebSocketHandler(const char* endpoint) const {
	auto splits = split(endpoint, '?');
	auto iter = _webSocketHandlerMap.find(splits[0]);
	if (iter == _webSocketHandlerMap.end()) {
		return std::shared_ptr<WebSocket::Handler>();
	}
	return iter->second.handler;
}

std::shared_ptr<PageHandler> Server::getPageHandler() const {
	return _pageHandler;
}

void Server::schedule(std::shared_ptr<Runnable> runnable) {
  {
  	LockGuard lock(_pendingRunnableMutex);
  	_pendingRunnables.push_back(runnable);
  }
	uint64_t one = 1;
	if (_pipes[1] != -1 && ::write(_pipes[1], &one, sizeof(one)) == -1) {
    if (errno != EAGAIN && errno != EWOULDBLOCK) {
  		LS_ERROR(_logger, "Unable to post a wake event: " << getLastError());
    }
	}
}

std::shared_ptr<Server::Runnable> Server::popNextRunnable() {
	LockGuard lock(_pendingRunnableMutex);
	std::shared_ptr<Runnable> runnable;
	if (!_pendingRunnables.empty()) {
		runnable = _pendingRunnables.front();
		_pendingRunnables.pop_front();
	}
	return runnable;
}

std::string Server::getStatsDocument() const {
	std::ostringstream doc;
	doc << "clear();" << std::endl;
	for (auto it = _connections.begin(); it != _connections.end(); ++it) {
		doc << "connection({";
		auto connection = it->first;
		jsonKeyPairToStream(doc,
				"since", EpochTimeAsLocal(it->second),
				"fd", connection->getFd(),
				"id", reinterpret_cast<uint64_t>(connection),
				"uri", connection->getRequestUri(),
				"addr", formatAddress(connection->getRemoteAddress()),
				"user", connection->credentials()->username,
				"input", connection->inputBufferSize(),
				"read", connection->bytesReceived(),
				"output", connection->outputBufferSize(),
				"written", connection->bytesSent()
				);
		doc << "});" << std::endl;
	}
	return doc.str();
}

void Server::setLameConnectionTimeoutSeconds(int seconds) {
	LS_INFO(_logger, "Setting lame connection timeout to " << seconds);
	_lameConnectionTimeoutSeconds = seconds;
}

void Server::setMaxKeepAliveDrops(int maxKeepAliveDrops) {
	LS_INFO(_logger, "Setting max keep alive drops to " << maxKeepAliveDrops);
	_maxKeepAliveDrops = maxKeepAliveDrops;
}

void Server::checkThread() const {
	auto thisTid = gettid();
	if (thisTid != _threadId) {
		std::ostringstream o;
		o << "SeaSocks called on wrong thread : " << thisTid << " instead of " << _threadId;
		LS_SEVERE(_logger, o.str());
		throw std::runtime_error(o.str());
	}
}

}  // namespace SeaSocks
