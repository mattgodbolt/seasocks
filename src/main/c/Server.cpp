// Copyright (c) 2013, Matt Godbolt
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimer.
//
// Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include "internal/LogStream.h"

#include "seasocks/Connection.h"
#include "seasocks/Logger.h"
#include "seasocks/Server.h"
#include "seasocks/PageHandler.h"
#include "seasocks/StringUtil.h"
#include "seasocks/util/Json.h"

#include <netinet/in.h>
#include <netinet/tcp.h>

#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/syscall.h>

#include <memory>
#include <stdexcept>
#include <string.h>
#include <unistd.h>

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

namespace seasocks {

Server::Server(std::shared_ptr<Logger> logger)
: _logger(logger), _listenSock(-1), _epollFd(-1), _eventFd(-1),
  _maxKeepAliveDrops(0),
  _lameConnectionTimeoutSeconds(DefaultLameConnectionTimeoutSeconds),
  _nextDeadConnectionCheck(0), _threadId(0), _terminate(false),
  _expectedTerminate(false) {

    _epollFd = epoll_create(10);
    if (_epollFd == -1) {
        LS_ERROR(_logger, "Unable to create epoll: " << getLastError());
        return;
    }

    _eventFd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (_eventFd == -1) {
        LS_ERROR(_logger, "Unable to create event FD: " << getLastError());
        return;
    }

    epoll_event eventWake = { EPOLLIN, { &_eventFd } };
    if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, _eventFd, &eventWake) == -1) {
        LS_ERROR(_logger, "Unable to add wake socket to epoll: " << getLastError());
        return;
    }
}

Server::~Server() {
    LS_INFO(_logger, "Server destruction");
    shutdown();
    // Only shut the eventfd and epoll at the very end
    if (_eventFd != -1) {
        close(_eventFd);
    }
    if (_epollFd != -1) {
        close(_epollFd);
    }
}

void Server::shutdown() {
    // Stop listening to any further incoming connections.
    if (_listenSock != -1) {
        close(_listenSock);
        _listenSock = -1;
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
    _expectedTerminate = true;
    _terminate = true;
    uint64_t one = 1;
    if (_eventFd != -1 && ::write(_eventFd, &one, sizeof(one)) == -1) {
        LS_ERROR(_logger, "Unable to post a wake event: " << getLastError());
    }
}

bool Server::startListening(int port) {
    return startListening(INADDR_ANY, port);
}

bool Server::startListening(uint32_t hostAddr, int port) {
    if (_epollFd == -1 || _eventFd == -1) {
        LS_ERROR(_logger, "Unable to serve, did not initialize properly.");
        return false;
    }

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
    sock.sin_addr.s_addr = htonl(hostAddr);
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

    char buf[1024];
    ::gethostname(buf, sizeof(buf));
    LS_INFO(_logger, "Listening on http://" << buf << ":" << port << "/");

    return true;
}

void Server::handlePipe() {
    uint64_t dummy;
    while (::read(_eventFd, &dummy, sizeof(dummy)) != -1) {
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

void Server::checkAndDispatchEpoll(int epollMillis) {
    const int maxEvents = 256;
    epoll_event events[maxEvents];

    std::list<Connection*> toBeDeleted;
    int numEvents = epoll_wait(_epollFd, events, maxEvents, epollMillis);
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
        } else if (events[i].data.ptr == &_eventFd) {
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

void Server::setStaticPath(const char* staticPath) {
    LS_INFO(_logger, "Serving content from " << staticPath);
    _staticPath = staticPath;
}

bool Server::serve(const char* staticPath, int port) {
    setStaticPath(staticPath);
    if (!startListening(port)) {
        return false;
    }

    return loop();
}

bool Server::loop() {
    if (_listenSock == -1) {
        LS_ERROR(_logger, "Server not initialised");
        return false;
    }

    // Stash away "the" server thread id.
    _threadId = gettid();

    while (!_terminate) {
        // Always process events first to catch start up events.
        processEventQueue();
        checkAndDispatchEpoll(EpollTimeoutMillis);
    }
    // Reasonable effort to ensure anything enqueued during terminate has a chance to run.
    processEventQueue();
    LS_INFO(_logger, "Server terminating");
    shutdown();
    return _expectedTerminate;
}

Server::PollResult Server::poll(int millis) {
    // Grab the thread ID on the first poll.
    if (_threadId == 0) _threadId = gettid();
    if (_threadId != gettid()) {
        LS_ERROR(_logger, "poll() called from the wrong thread");
        return PollResult::Error;
    }
    if (_listenSock == -1) {
        LS_ERROR(_logger, "Server not initialised");
        return PollResult::Error;
    }
    processEventQueue();
    checkAndDispatchEpoll(millis);
    if (!_terminate) return PollResult::Continue;

    // Reasonable effort to ensure anything enqueued during terminate has a chance to run.
    processEventQueue();
    LS_INFO(_logger, "Server terminating");
    shutdown();

    return _expectedTerminate ? PollResult::Terminated : PollResult::Error;
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
    Connection* newConnection = new Connection(_logger, *this, fd, address);
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

void Server::addPageHandler(std::shared_ptr<PageHandler> handler) {
    _pageHandlers.emplace_back(handler);
}

bool Server::isCrossOriginAllowed(const std::string &endpoint) const {
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

void Server::execute(std::shared_ptr<Runnable> runnable) {
    std::unique_lock<decltype(_pendingRunnableMutex)> lock(_pendingRunnableMutex);
    _pendingRunnables.push_back(runnable);
    lock.unlock();

    uint64_t one = 1;
    if (_eventFd != -1 && ::write(_eventFd, &one, sizeof(one)) == -1) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            LS_ERROR(_logger, "Unable to post a wake event: " << getLastError());
        }
    }
}

std::shared_ptr<Server::Runnable> Server::popNextRunnable() {
    std::lock_guard<decltype(_pendingRunnableMutex)> lock(_pendingRunnableMutex);
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
                "user", connection->credentials() ?
                        connection->credentials()->username : "(not authed)",
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
        o << "seasocks called on wrong thread : " << thisTid << " instead of " << _threadId;
        LS_SEVERE(_logger, o.str());
        throw std::runtime_error(o.str());
    }
}

std::shared_ptr<Response> Server::handle(const Request &request) {
    for (auto handler : _pageHandlers) {
        auto result = handler->handle(request);
        if (result != Response::unhandled()) return result;
    }
    return Response::unhandled();
}

}  // namespace seasocks
