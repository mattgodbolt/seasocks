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

#include "seasocks/PrintfLogger.h"
#include "seasocks/Server.h"
#include "seasocks/StringUtil.h"
#include "seasocks/WebSocket.h"

#include <cstring>
#include <iostream>
#include <memory>
#include <set>
#include <sstream>
#include <string>

/* Simple server that echo any text or binary WebSocket messages back. */

using namespace seasocks;

class EchoHandler : public WebSocket::Handler {
public:
    virtual void onConnect(WebSocket* /*connection*/) override {
    }

    virtual void onData(WebSocket* connection, const uint8_t* data, size_t length) override {
        connection->send(data, length); // binary
    }

    virtual void onData(WebSocket* connection, const char* data) override {
        connection->send(data); // text
    }

    virtual void onDisconnect(WebSocket* /*connection*/) override {
    }
};

int main(int /*argc*/, const char* /*argv*/[]) {
    auto logger = std::make_shared<PrintfLogger>(Logger::Level::Debug);

    Server server(logger);
    auto handler = std::make_shared<EchoHandler>();
    server.addWebSocketHandler("/", handler);
    server.serve("/dev/null", 8000);
    return 0;
}
