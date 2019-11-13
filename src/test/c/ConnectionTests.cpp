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

#include "MockServerImpl.h"
#include "seasocks/Connection.h"
#include "seasocks/IgnoringLogger.h"

#include <catch2/catch.hpp>

#include <iostream>
#include <sstream>
#include <cstring>
#include <string>

using namespace seasocks;

class TestHandler : public WebSocket::Handler {
public:
    int _stage;
    TestHandler()
            : _stage(0) {
    }
    ~TestHandler() {
        if (_stage != 2) {
            FAIL("Invalid state");
        }
    }
    void onConnect(WebSocket*) override {
    }
    void onData(WebSocket*, const char* data) override {
        if (_stage == 0) {
            CHECK(strcmp(data, "a") == 0);
        } else if (_stage == 1) {
            CHECK(strcmp(data, "b") == 0);
        } else {
            FAIL("unexpected state");
        }
        ++_stage;
    }
    void onDisconnect(WebSocket*) override {
    }
};

TEST_CASE("Connection tests", "[ConnectionTests]") {
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = 0x1234;
    addr.sin_addr.s_addr = 0x01020304;
    auto logger = std::make_shared<IgnoringLogger>();
    MockServerImpl mockServer;
    Connection connection(logger, mockServer, -1, addr);

    SECTION("should break hixie messages apart in same buffer") {
        connection.setHandler(std::make_shared<TestHandler>());
        uint8_t foo[] = {0x00, 'a', 0xff, 0x00, 'b', 0xff};
        connection.getInputBuffer().assign(&foo[0], &foo[sizeof(foo)]);
        connection.handleHixieWebSocket();
    }
    SECTION("shouldAcceptMultipleConnectionTypes") {
        const uint8_t message[] = "GET /ws-test HTTP/1.1\r\nConnection: keep-alive, Upgrade\r\nUpgrade: websocket\r\n\r\n";
        connection.getInputBuffer().assign(&message[0], &message[sizeof(message)]);
        mockServer.handlers["/ws-test"] = std::shared_ptr<WebSocket::Handler>();
        connection.handleNewData();
    }
}
