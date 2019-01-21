// Copyright (c) 2013-2017, Matt Godbolt
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

#pragma once

#include "seasocks/ServerImpl.h"
#include "seasocks/WebSocket.h"

#include <sys/types.h>

#include <atomic>
#include <cstdint>
#include <functional>
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
class Request;
class Response;

class Server : private ServerImpl {
public:
    explicit Server(std::shared_ptr<Logger> logger);
    virtual ~Server();

    void addPageHandler(std::shared_ptr<PageHandler> handler);

    void addWebSocketHandler(const char* endpoint, std::shared_ptr<WebSocket::Handler> handler,
                             bool allowCrossOriginRequests = false);

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

    // Starts listening on a unix domain socket.
    // Returns true if all was ok.
    bool startListeningUnix(const char* socketPath);

    // Sets the path to server static content from.
    void setStaticPath(const char* staticPath);

    // Loop (until terminate called from another thread).
    // Returns true if terminate() was used to exit the loop, false if there
    // was an error.
    bool loop();

    // Runs a single iteration of the main loop, blocking for a given time.
    // Returns immediately if terminate() has been called. Must be consistently
    // called from the same thread. Returns an enum describing why it returned.
    enum class PollResult {
        Continue,
        Terminated,
        Error,
    };
    PollResult poll(int millisToBlock);

    // Returns a file descriptor that can be polled for changes (e.g. by
    // placing it in an epoll set. The poll() method above only need be called
    // when this file descriptor is readable.
    int fd() const {
        return _epollFd;
    }

    // Terminate any loop() or poll(). May be called from any thread.
    void terminate();

    // If we haven't heard anything ever on a connection for this long, kill it.
    // This is possibly caused by bad WebSocket implementation in Chrome.
    void setLameConnectionTimeoutSeconds(int seconds);

    // Sets the maximum number of TCP level keepalives that we can miss before
    // we let the OS consider the connection dead. We configure keepalives every second,
    // so this is also the minimum number of seconds it takes to notice a badly-behaved
    // dead connection, e.g. a laptop going into sleep mode or a hard-crashed machine.
    // A value of 0 disables keep alives, which is the default.
    void setMaxKeepAliveDrops(int maxKeepAliveDrops);

    // Set the maximum amount of data we'll buffer for a client before we close the
    // connection assuming the client can't keep up with the data rate. Default
    // is available here too.
    static constexpr size_t DefaultClientBufferSize = 16 * 1024 * 1024u;
    void setClientBufferSize(size_t bytesToBuffer);
    size_t clientBufferSize() const override {
        return _clientBufferSize;
    }

    void setPerMessageDeflateEnabled(bool enabled);
    bool getPerMessageDeflateEnabled() {
        return _perMessageDeflateEnabled;
    }

    class Runnable {
    public:
        virtual ~Runnable() = default;
        virtual void run() = 0;
    };
    // Execute a task on the Seasocks thread.
    void execute(std::shared_ptr<Runnable> runnable);
    using Executable = std::function<void()>;
    void execute(Executable toExecute);

private:
    // From ServerImpl
    virtual void remove(Connection* connection) override;
    virtual bool subscribeToWriteEvents(Connection* connection) override;
    virtual bool unsubscribeFromWriteEvents(Connection* connection) override;
    virtual const std::string& getStaticPath() const override {
        return _staticPath;
    }
    virtual std::shared_ptr<WebSocket::Handler> getWebSocketHandler(const char* endpoint) const override;
    virtual bool isCrossOriginAllowed(const std::string& endpoint) const override;
    virtual std::shared_ptr<Response> handle(const Request& request) override;
    virtual std::string getStatsDocument() const override;
    virtual void checkThread() const override;
    virtual Server& server() override {
        return *this;
    }

    bool makeNonBlocking(int fd) const;
    bool configureSocket(int fd) const;
    void handleAccept();
    void processEventQueue();
    void runExecutables();

    void shutdown();

    void checkAndDispatchEpoll(int epollMillis);
    void handlePipe();
    enum class NewState { KeepOpen,
                          Close };
    NewState handleConnectionEvents(Connection* connection, uint32_t events);

    // Connections, mapped to initial connection time.
    std::map<Connection*, time_t> _connections;
    std::shared_ptr<Logger> _logger;
    int _listenSock;
    int _epollFd;
    int _eventFd;
    int _maxKeepAliveDrops;
    int _lameConnectionTimeoutSeconds;
    size_t _clientBufferSize;
    time_t _nextDeadConnectionCheck;

    // Compression settings
    bool _perMessageDeflateEnabled = false;

    struct WebSocketHandlerEntry {
        std::shared_ptr<WebSocket::Handler> handler;
        bool allowCrossOrigin;
    };
    typedef std::unordered_map<std::string, WebSocketHandlerEntry> WebSocketHandlerMap;
    WebSocketHandlerMap _webSocketHandlerMap;

    std::list<std::shared_ptr<PageHandler>> _pageHandlers;

    std::mutex _pendingExecutableMutex;
    std::list<Executable> _pendingExecutables;

    pid_t _threadId;

    std::string _staticPath;
    std::atomic<bool> _terminate;
    std::atomic<bool> _expectedTerminate;
};

} // namespace seasocks
