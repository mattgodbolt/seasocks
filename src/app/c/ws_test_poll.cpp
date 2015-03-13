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

// An extraordinarily simple test which presents a web page with some buttons.
// Clicking on the numbered button increments the number, which is visible to
// other connected clients.  WebSockets are used to do this: by the rather
// suspicious means of sending raw JavaScript commands to be executed on other
// clients.

// Same as ws_test, but uses the poll() method and a separate epoll set to
// demonstrate how Seasocks can be used with another polling system.

#include "seasocks/PrintfLogger.h"
#include "seasocks/Server.h"
#include "seasocks/StringUtil.h"
#include "seasocks/WebSocket.h"
#include "seasocks/util/Json.h"

#include <cstring>
#include <iostream>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <sys/epoll.h>

using namespace seasocks;
using namespace std;

class MyHandler: public WebSocket::Handler {
public:
    MyHandler(Server* server) : _server(server), _currentValue(0) {
        setValue(1);
    }

    virtual void onConnect(WebSocket* connection) {
        _connections.insert(connection);
        connection->send(_currentSetValue.c_str());
        cout << "Connected: " << connection->getRequestUri()
                << " : " << formatAddress(connection->getRemoteAddress())
                << endl;
        cout << "Credentials: " << *(connection->credentials()) << endl;
    }

    virtual void onData(WebSocket* connection, const char* data) {
        if (0 == strcmp("die", data)) {
            _server->terminate();
            return;
        }
        if (0 == strcmp("close", data)) {
            cout << "Closing.." << endl;
            connection->close();
            cout << "Closed." << endl;
            return;
        }

        int value = atoi(data) + 1;
        if (value > _currentValue) {
            setValue(value);
            for (auto connection : _connections) {
                connection->send(_currentSetValue.c_str());
            }
        }
    }

    virtual void onDisconnect(WebSocket* connection) {
        _connections.erase(connection);
        cout << "Disconnected: " << connection->getRequestUri()
                << " : " << formatAddress(connection->getRemoteAddress())
                << endl;
    }

private:
    set<WebSocket*> _connections;
    Server* _server;
    int _currentValue;
    string _currentSetValue;

    void setValue(int value) {
        _currentValue = value;
        _currentSetValue = makeExecString("set", _currentValue);
    }
};

int main(int argc, const char* argv[]) {
    shared_ptr<Logger> logger(new PrintfLogger(Logger::DEBUG));

    Server server(logger);

    shared_ptr<MyHandler> handler(new MyHandler(&server));
    server.addWebSocketHandler("/ws", handler);
    server.setStaticPath("src/ws_test_web");
    if (!server.startListening(9090)) {
        cerr << "couldn't start listening" << endl;
        return 1;
    }
    int myEpoll = epoll_create(10);
    epoll_event wakeSeasocks = { EPOLLIN|EPOLLOUT|EPOLLERR, { &server } };
    epoll_ctl(myEpoll, EPOLL_CTL_ADD, server.fd(), &wakeSeasocks);

    // Also poll stdin
    epoll_event wakeStdin = { EPOLLIN, { nullptr } };
    epoll_ctl(myEpoll, EPOLL_CTL_ADD, STDIN_FILENO, &wakeStdin);
    auto prevFlags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, prevFlags | O_NONBLOCK);

    cout << "Will echo anything typed in stdin: " << flush;
    while (true) {
        constexpr auto maxEvents = 2;
        epoll_event events[maxEvents];
        auto res = epoll_wait(myEpoll, events, maxEvents, -1);
        if (res < 0) {
            cerr << "epoll returned an error" << endl;
            return 1;
        }
        for (auto i = 0; i < res; ++i) {
            if (events[i].data.ptr == &server) {
                auto seasocksResult = server.poll(0);
                if (seasocksResult == Server::PollResult::Terminated) return 0;
                if (seasocksResult == Server::PollResult::Error) return 1;
            } else if (events[i].data.ptr == nullptr) {
                // Echo stdin to stdout to show we can read from that too.
                for (;;) {
                    char buf[1024];
                    auto numRead = ::read(STDIN_FILENO, buf, sizeof(buf));
                    if (numRead < 0) {
                        if (errno != EWOULDBLOCK && errno != EAGAIN) {
                            cerr << "Error reading stdin" << endl;
                            return 1;
                        }
                        break;
                    } else if (numRead > 0) {
                        auto written = write(STDOUT_FILENO, buf, numRead);
                        if (written != numRead) {
                            cerr << "Truncated write" << endl;
                        }
                    } else if (numRead == 0) {
                        cerr << "EOF on stdin" << endl;
                        return 0;
                    }
                }
            }
        }
    }
    return 0;
}
