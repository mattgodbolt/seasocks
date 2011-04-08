#include "seasocks/server.h"

#include "seasocks/connection.h"
#include "seasocks/stringutil.h"
#include "seasocks/logger.h"

#include <string.h>

#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <memory>

namespace SeaSocks {

Server::Server(boost::shared_ptr<Logger> logger)
	: _logger(logger), _listenSock(-1), _epollFd(-1), _staticPath(NULL),_terminate(false) {
	_pipes[0] = _pipes[1] = -1;
}

Server::~Server() {
	_logger->info("Server shutting down");
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
		_logger->error("Unable to make socket non-blocking: %s", getLastError());
		return false;
	}
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yesPlease, sizeof(yesPlease)) == -1) {
		_logger->error("Unable to set reuse socket option: %s", getLastError());
		return false;
	}
	return true;
}

void Server::terminate() {
    _terminate = true;
    uint64_t one = 1;
	if (::write(_pipes[1], &one, sizeof(one)) == -1) {
		_logger->error("Unable to post a wake event: %s", getLastError());
	}
}

void Server::serve(const char* staticPath, int port) {
	_staticPath = staticPath;
	_listenSock = socket(AF_INET, SOCK_STREAM, 0);
	if (_listenSock == -1) {
		_logger->error("Unable to create listen socket: %s", getLastError());
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
		_logger->error("Unable to bind socket: %s", getLastError());
		return;
	}
	if (listen(_listenSock, 5) == -1) {
		_logger->error("Unable to listen on socket: %s", getLastError());
		return;
	}

	_epollFd = epoll_create(10);
	if (_epollFd == -1) {
		_logger->error("Unable to create epoll: %s", getLastError());
		return;
	}

	/* TASK: once RH5 is dead and gone, use: _wakeFd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC); instead. It's lower overhead than this. */
	if (pipe(_pipes) != 0) {
		_logger->error("Unable to create event signal pipe: %s", getLastError());
		return;
	}

	epoll_event event = { EPOLLIN, this };
	if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, _listenSock, &event) == -1) {
		_logger->error("Unable to add listen socket to epoll: %s", getLastError());
		return;
	}

	epoll_event eventWake = { EPOLLIN, &_pipes[0] };
	if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, _pipes[0], &eventWake) == -1) {
		_logger->error("Unable to add wake socket to epoll: %s", getLastError());
		return;
	}

	_logger->info("Listening on http://%s", formatAddress(sock));

	const int maxEvents = 20;
	epoll_event events[maxEvents];

	for (;;) {
		int numEvents = epoll_wait(_epollFd, events, maxEvents, -1);
		if (numEvents == -1) {
			if (errno == EINTR) continue;
			_logger->error("Error from epoll_wait: %s", getLastError());
			return;
		}
		for (int i = 0; i < numEvents; ++i) {
			if (events[i].data.ptr == this) {
				handleAccept();
			} else if (events[i].data.ptr == &_pipes[0]) {
				uint64_t dummy;
				if (::read(_pipes[0], &dummy, sizeof(dummy)) == -1) {
					_logger->error("Error from wakeFd read: %s", getLastError());
				}
				// It's a "wake up" event; just process any runnables.
			} else {
				auto connection = reinterpret_cast<Connection*>(events[i].data.ptr);
				bool keepAlive = true;
				if (events[i].events & EPOLLIN) {
					keepAlive &= connection->handleDataReadyForRead();
				}
				if (events[i].events & EPOLLOUT) {
					keepAlive &= connection->handleDataReadyForWrite();
				}
				if (!keepAlive) {
					_logger->debug("Deleting connection: %s", formatAddress(connection->getAddress()));
					delete connection;
				}
			}
		}
		for (;;) {
			boost::shared_ptr<Runnable> runnable = popNextRunnable();
			if (!runnable) break;
			runnable->run();
		}
        if (_terminate) break;
	}
}

void Server::handleAccept() {
	sockaddr_in address;
	socklen_t addrLen = sizeof(address);
	int fd = ::accept(_listenSock,
			reinterpret_cast<sockaddr*>(&address),
			&addrLen);
	if (fd == -1) {
		_logger->error("Unable to accept: %s", getLastError());
		return;
	}
	struct linger linger;
	linger.l_linger = 500; // 5 seconds
	linger.l_onoff = true;
	if (setsockopt(fd, SOL_SOCKET, SO_LINGER, &linger, sizeof(linger)) == -1) {
		_logger->error("Unable to set linger socket option: %s", getLastError());
		::close(fd);
		return;
	}
	if (!configureSocket(fd)) {
		::close(fd);
		return;
	}
	_logger->debug("%s : Accepted on descriptor %d", formatAddress(address), fd);
	// TODO: track all connections?
	Connection* newConnection = new Connection(_logger, this, fd, address);
	epoll_event event = { EPOLLIN, newConnection };
	if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, fd, &event) == -1) {
		_logger->error("Unable to add socket to epoll: %s", getLastError());
		delete newConnection;
		::close(fd);
		return;
	}
}

void Server::unsubscribeFromAllEvents(Connection* connection) {
	epoll_event event = { 0, connection };
	if (epoll_ctl(_epollFd, EPOLL_CTL_DEL, connection->getFd(), &event) == -1) {
		_logger->error("Unable to remove from epoll: %s", getLastError());
	}
}

bool Server::subscribeToWriteEvents(Connection* connection) {
	epoll_event event = { EPOLLIN | EPOLLOUT, connection };
	if (epoll_ctl(_epollFd, EPOLL_CTL_MOD, connection->getFd(), &event) == -1) {
		_logger->error("Unable to subscribe to write events: %s", getLastError());
		return false;
	}
	return true;
}

bool Server::unsubscribeFromWriteEvents(Connection* connection) {
	epoll_event event = { EPOLLIN, connection };
	if (epoll_ctl(_epollFd, EPOLL_CTL_MOD, connection->getFd(), &event) == -1) {
		_logger->error("Unable to unsubscribe from write events: %s", getLastError());
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
	if (::write(_pipes[1], &one, sizeof(one)) == -1) {
		_logger->error("Unable to post a wake event: %s", getLastError());
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

}  // namespace SeaSocks
