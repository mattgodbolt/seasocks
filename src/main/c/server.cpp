#include "seasocks/server.h"

#include "seasocks/connection.h"
#include "seasocks/stringutil.h"
#include "seasocks/logger.h"
#include "seasocks/json.h"

#include "internal/LogStream.h"

#include <string.h>

#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <memory>

namespace SeaSocks {

Server::Server(boost::shared_ptr<Logger> logger)
	: _logger(logger), _listenSock(-1), _epollFd(-1), _terminate(false) {
	_pipes[0] = _pipes[1] = -1;
	_sso = boost::shared_ptr<SsoAuthenticator>();
}

void Server::enableSingleSignOn(SsoOptions ssoOptions) {
	_sso.reset(new SsoAuthenticator(ssoOptions));
}

Server::~Server() {
	_logger->info("Server shutting down");
	while (!_connections.empty()) {
		// Deleting the connection closes it and removes it from 'this'.
		delete *_connections.begin();
	}
	if (_listenSock != -1) {
		close(_listenSock);
	}
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

bool Server::configureSocket(int fd) const {
	int yesPlease = 1;
	if (ioctl(fd, FIONBIO, &yesPlease) != 0) {
		LS_ERROR(_logger, "Unable to make socket non-blocking: " << getLastError());
		return false;
	}
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yesPlease, sizeof(yesPlease)) == -1) {
		LS_ERROR(_logger, "Unable to set reuse socket option: " << getLastError());
		return false;
	}
	return true;
}

void Server::terminate() {
    _terminate = true;
    uint64_t one = 1;
	if (::write(_pipes[1], &one, sizeof(one)) == -1) {
		LS_ERROR(_logger, "Unable to post a wake event: " << getLastError());
	}
}

void Server::serve(const char* staticPath, int port) {
	_staticPath = staticPath;
	_listenSock = socket(AF_INET, SOCK_STREAM, 0);
	if (_listenSock == -1) {
		LS_ERROR(_logger, "Unable to create listen socket: " << getLastError());
		return;
	}
	if (!configureSocket(_listenSock)) {
		return;
	}
	sockaddr_in sock;
	memset(&sock, 0, sizeof(sock));
	sock.sin_port = htons(port);
	sock.sin_addr.s_addr = INADDR_ANY;
	sock.sin_family = AF_INET;
	if (bind(_listenSock, reinterpret_cast<const sockaddr*>(&sock), sizeof(sock)) == -1) {
		LS_ERROR(_logger, "Unable to bind socket: " << getLastError());
		return;
	}
	if (listen(_listenSock, 5) == -1) {
		LS_ERROR(_logger, "Unable to listen on socket: " << getLastError());
		return;
	}

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

	epoll_event event = { EPOLLIN, this };
	if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, _listenSock, &event) == -1) {
		LS_ERROR(_logger, "Unable to add listen socket to epoll: " << getLastError());
		return;
	}

	epoll_event eventWake = { EPOLLIN, &_pipes[0] };
	if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, _pipes[0], &eventWake) == -1) {
		LS_ERROR(_logger, "Unable to add wake socket to epoll: " << getLastError());
		return;
	}

	LS_INFO(_logger, "Listening on http://" << formatAddress(sock));

	const int maxEvents = 20;
	epoll_event events[maxEvents];

	processEventQueue(); // For any events before startup.
	for (;;) {
		int numEvents = epoll_wait(_epollFd, events, maxEvents, -1);
		if (numEvents == -1) {
			if (errno == EINTR) continue;
			LS_ERROR(_logger, "Error from epoll_wait: " << getLastError());
			return;
		}
		for (int i = 0; i < numEvents; ++i) {
			if (events[i].data.ptr == this) {
				handleAccept();
			} else if (events[i].data.ptr == &_pipes[0]) {
				uint64_t dummy;
				if (::read(_pipes[0], &dummy, sizeof(dummy)) == -1) {
					LS_ERROR(_logger, "Error from wakeFd read: " << getLastError());
				}
				// It's a "wake up" event; just process any runnables.
			} else {
				auto connection = reinterpret_cast<Connection*>(events[i].data.ptr);
				bool keepAlive = true;
				if (events[i].events & EPOLLERR) {
					LS_WARNING(_logger, "Got epoll error on connection: " << formatAddress(connection->getRemoteAddress()));
					keepAlive = false;
				}
				if (keepAlive && events[i].events & EPOLLIN) {
					keepAlive = connection->handleDataReadyForRead();
				}
				if (keepAlive && (events[i].events & EPOLLOUT)) {
					keepAlive = connection->handleDataReadyForWrite();
				}
				if (!keepAlive) {
					LS_DEBUG(_logger, "Deleting connection: " << formatAddress(connection->getRemoteAddress()));
					delete connection;
				}
			}
		}
		processEventQueue();
		if (_terminate) break;
	}
}

void Server::processEventQueue() {
	for (;;) {
		boost::shared_ptr<Runnable> runnable = popNextRunnable();
		if (!runnable) break;
		runnable->run();
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
	struct linger linger;
	linger.l_linger = 500; // 5 seconds
	linger.l_onoff = true;
	if (setsockopt(fd, SOL_SOCKET, SO_LINGER, &linger, sizeof(linger)) == -1) {
		LS_ERROR(_logger, "Unable to set linger socket option: " << getLastError());
		::close(fd);
		return;
	}
	if (!configureSocket(fd)) {
		::close(fd);
		return;
	}
	_logger->debug("%s : Accepted on descriptor %d", formatAddress(address).c_str(), fd);
	Connection* newConnection = new Connection(_logger, this, fd, address, _sso);
	epoll_event event = { EPOLLIN | EPOLLERR, newConnection };
	if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, fd, &event) == -1) {
		LS_ERROR(_logger, "Unable to add socket to epoll: " << getLastError());
		delete newConnection;
		::close(fd);
		return;
	}
	_connections.insert(newConnection);
}

void Server::remove(Connection* connection) {
	epoll_event event = { 0, connection };
	if (epoll_ctl(_epollFd, EPOLL_CTL_DEL, connection->getFd(), &event) == -1) {
		LS_ERROR(_logger, "Unable to remove from epoll: " << getLastError());
	}
	_connections.erase(connection);
}

bool Server::subscribeToWriteEvents(Connection* connection) {
	epoll_event event = { EPOLLIN | EPOLLERR | EPOLLOUT, connection };
	if (epoll_ctl(_epollFd, EPOLL_CTL_MOD, connection->getFd(), &event) == -1) {
		LS_ERROR(_logger, "Unable to subscribe to write events: " << getLastError());
		return false;
	}
	return true;
}

bool Server::unsubscribeFromWriteEvents(Connection* connection) {
	epoll_event event = { EPOLLIN | EPOLLERR, connection };
	if (epoll_ctl(_epollFd, EPOLL_CTL_MOD, connection->getFd(), &event) == -1) {
		LS_ERROR(_logger, "Unable to unsubscribe from write events: " << getLastError());
		return false;
	}
	return true;
}

void Server::addWebSocketHandler(const char* endpoint, boost::shared_ptr<WebSocket::Handler> handler,
		bool allowCrossOriginRequests) {
	_handlerMap[endpoint] = { handler, allowCrossOriginRequests };
}

bool Server::isCrossOriginAllowed(const char* endpoint) const {
	auto iter = _handlerMap.find(endpoint);
	if (iter == _handlerMap.end()) {
		return false;
	}
	return iter->second.allowCrossOrigin;
}

boost::shared_ptr<WebSocket::Handler> Server::getWebSocketHandler(const char* endpoint) const {
	auto iter = _handlerMap.find(endpoint);
	if (iter == _handlerMap.end()) {
		return boost::shared_ptr<WebSocket::Handler>();
	}
	return iter->second.handler;
}

void Server::schedule(boost::shared_ptr<Runnable> runnable) {
	LockGuard lock(_pendingRunnableMutex);
	_pendingRunnables.push_back(runnable);
	uint64_t one = 1;
	if (_pipes[1] != -1 && ::write(_pipes[1], &one, sizeof(one)) == -1) {
		LS_ERROR(_logger, "Unable to post a wake event: " << getLastError());
	}
}

boost::shared_ptr<Server::Runnable> Server::popNextRunnable() {
	LockGuard lock(_pendingRunnableMutex);
	boost::shared_ptr<Runnable> runnable;
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
		auto connection = *it;
		jsonKeyPairToStream(doc,
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

}  // namespace SeaSocks
