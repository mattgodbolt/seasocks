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

#include "seasocks/PageHandler.h"
#include "seasocks/PrintfLogger.h"
#include "seasocks/Server.h"
#include "seasocks/WebSocket.h"
#include "seasocks/StringUtil.h"

#include <memory>
#include <set>
#include <string>

// Simple chatroom server, showing how one might use authentication.

using namespace seasocks;

namespace {

struct Handler : WebSocket::Handler {
    std::set<WebSocket*> _cons;

    void onConnect(WebSocket* con) override {
        _cons.insert(con);
        send(con->credentials()->username + " has joined");
    }
    void onDisconnect(WebSocket* con) override {
        _cons.erase(con);
        send(con->credentials()->username + " has left");
    }

    void onData(WebSocket* con, const char* data) override {
        send(con->credentials()->username + ": " + data);
    }

    void send(const std::string& msg) {
        for (auto* con : _cons) {
            con->send(msg);
        }
    }
};

struct MyAuthHandler : PageHandler {
    std::shared_ptr<Response> handle(const Request& request) override {
        // Here one would handle one's authentication system, for example;
        // * check to see if the user has a trusted cookie: if so, accept it.
        // * if not, redirect to a login handler page, and await a redirection
        //   back here with relevant URL parameters indicating success. Then,
        //   set the cookie.
        // For this example, we set the user's authentication information purely
        // from their connection.
        request.credentials()->username = formatAddress(request.getRemoteAddress());
        return Response::unhandled(); // cause next handler to run
    }
};

}

int main(int /*argc*/, const char* /*argv*/[]) {
    Server server(std::make_shared<PrintfLogger>());
    server.addPageHandler(std::make_shared<MyAuthHandler>());
    server.addWebSocketHandler("/chat", std::make_shared<Handler>());
    server.serve("src/ws_chatroom_web", 9000);
    return 0;
}
