#include "server.h"

#include "connection.h"
#include "stringutil.h"
#include "logger.h"

#include <string.h>

#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#include <memory>

namespace SeaSocks {

Server::Server(boost::shared_ptr<Logger> logger)
	: _logger(logger), _listenSock(-1), _epollFd(-1), _staticPath(NULL) {
}

Server::~Server() {
	_logger->info("Server shutting down");
	if (_listenSock != -1) {
		close(_listenSock);
	}
	if (_epollFd != -1) {
		close(_epollFd);
	}
}

void Server::serve(const char* staticPath, int port) {
	_staticPath = staticPath;
	_listenSock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
	if (_listenSock == -1) {
		_logger->error("Unable to create listen socket: %s", getLastError());
		return;
	}
	int yesPlease = 1;
	if (setsockopt(_listenSock, SOL_SOCKET, SO_REUSEADDR, &yesPlease, sizeof(yesPlease)) == -1) {
		_logger->error("Unable to set socket reuse: %s", getLastError());
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

	_epollFd = epoll_create1(EPOLL_CLOEXEC);
	if (_epollFd == -1) {
		_logger->error("Unable to create epoll: %s", getLastError());
		return;
	}

	epoll_event event;
	event.events = EPOLLIN;
	event.data.ptr = this;
	if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, _listenSock, &event) == -1) {
		_logger->error("Unable to add listen socket to epoll: %s", getLastError());
		return;
	}

	_logger->info("Listening on http://%s", formatAddress(sock));

	const int maxEvents = 20;
	epoll_event events[maxEvents];

	for (;;) {
		int numEvents = epoll_wait(_epollFd, events, maxEvents, -1);
		if (numEvents == -1) {
			_logger->error("Error from epoll_wait: %s", getLastError());
			return;
		}
		for (int i = 0; i < numEvents; ++i) {
			if (events[i].data.ptr == this) {
				handleAccept();
			} else {
				auto connection = reinterpret_cast<Connection*>(events[i].data.ptr);
				if (!connection->handleData(events[i].events)) {
					_logger->debug("Deleting connection: %d", connection->getFd());
					delete connection;
				}
			}
		}
	}
}

void Server::handleAccept() {
	std::auto_ptr<Connection> newConnection(new Connection(_logger, _staticPath));
	if (!newConnection->accept(_listenSock, _epollFd)) {
		_logger->error("Unable to accept: %s", getLastError());
		return;
	}
	_logger->debug("Accepted connection on fd %d", newConnection->getFd());
	newConnection.release();
}

}  // namespace SeaSocks
