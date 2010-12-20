#include "server.h"

#include "connection.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#include <memory>

namespace SeaSocks {

Server::Server() : _listenSock(-1), _epollFd(-1) {
}

Server::~Server() {
	if (_listenSock != -1) {
		close(_listenSock);
	}
	if (_epollFd != -1) {
		close(_epollFd);
	}
}

void Server::serve(int port) {
	_listenSock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
	if (_listenSock == -1) {
		perror("socket");
		return;
	}
	sockaddr_in sock;
	memset(&sock, 0, sizeof(sock));
	sock.sin_port = htons(port);
	sock.sin_addr.s_addr = INADDR_ANY;
	sock.sin_family = AF_INET;
	if (bind(_listenSock, reinterpret_cast<const sockaddr*>(&sock), sizeof(sock)) == -1) {
		perror("bind");
		return;
	}
	if (listen(_listenSock, 5) == -1) {
		perror("listen");
		return;
	}
	int yesPlease = 1;
	if (setsockopt(_listenSock, SOL_SOCKET, SO_REUSEADDR, &yesPlease, sizeof(yesPlease)) == -1) {
		perror("setsockopt");
		return;
	}

	_epollFd = epoll_create1(EPOLL_CLOEXEC);
	if (_epollFd == -1) {
		perror("epoll_create");
		return;
	}

	epoll_event event;
	event.events = EPOLLIN;
	event.data.ptr = this;
	if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, _listenSock, &event) == -1) {
		perror("epoll_ctl");
		return;
	}

	const int maxEvents = 20;
	epoll_event events[maxEvents];

	for (;;) {
		int numEvents = epoll_wait(_epollFd, events, maxEvents, -1);
		if (numEvents == -1) {
			perror("epoll_wait");
			return;
		}
		for (int i = 0; i < numEvents; ++i) {
			if (events[i].data.ptr == this) {
				handleAccept();
			} else {
				auto connection = reinterpret_cast<Connection*>(events[i].data.ptr);
				if (!connection->handleData(events[i].events)) {
					delete connection;
				}
			}
		}
	}
}

void Server::handleAccept() {
	std::auto_ptr<Connection> newConnection(new Connection());
	if (!newConnection->accept(_listenSock, _epollFd)) {
		perror("accept");
		// TODO: error
		return;
	}
	newConnection.release();
}

}  // namespace SeaSocks
