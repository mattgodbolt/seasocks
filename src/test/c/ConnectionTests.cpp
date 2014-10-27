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

#include "MockServerImpl.h"
#include "seasocks/Connection.h"
#include "seasocks/IgnoringLogger.h"

#include <gmock/gmock.h>

#include <iostream>
#include <sstream>
#include <string.h>
#include <string>

using namespace seasocks;

class TestHandler: public WebSocket::Handler {
public:
    int _stage;
    TestHandler() :
        _stage(0) {
    }
    ~TestHandler() {
        if (_stage != 2) {
            ADD_FAILURE() << "Invalid state";
        }
    }
    virtual void onConnect(WebSocket*) {
    }
    virtual void onData(WebSocket*, const char* data) {
        if (_stage == 0) {
            ASSERT_STREQ(data, "a");
        } else if (_stage == 1) {
            ASSERT_STREQ(data, "b");
        } else {
            FAIL() << "unexpected state";
        }
        ++_stage;
    }
    virtual void onDisconnect(WebSocket*) {
    }
};

TEST(ConnectionTests, shouldBreakHixieMessagesApartInSameBuffer) {
    sockaddr_in addr = { AF_INET, 0x1234, { 0x01020304 } };
    std::shared_ptr<Logger> logger(new IgnoringLogger);
    testing::NiceMock<MockServerImpl> mockServer;
    Connection connection(logger, mockServer, -1, addr);
    connection.setHandler(
            std::shared_ptr<WebSocket::Handler>(new TestHandler));
    uint8_t foo[] = { 0x00, 'a', 0xff, 0x00, 'b', 0xff };
    connection.getInputBuffer().assign(&foo[0], &foo[sizeof(foo)]);
    connection.handleHixieWebSocket();
    SUCCEED();
}

TEST(ConnectionTests, shouldAcceptMultipleConnectionTypes) {
    sockaddr_in addr = { AF_INET, 0x1234, { 0x01020304 } };
    std::shared_ptr<Logger> logger(new IgnoringLogger);
    testing::NiceMock<MockServerImpl> mockServer;
    Connection connection(logger, mockServer, -1, addr);
    const uint8_t message[] = "GET /ws-test HTTP/1.1\r\nConnection: keep-alive, Upgrade\r\nUpgrade: websocket\r\n\r\n";
    connection.getInputBuffer().assign(&message[0], &message[sizeof(message)]);
    EXPECT_CALL(mockServer, getWebSocketHandler(testing::StrEq("/ws-test")))
        .WillOnce(testing::Return(std::shared_ptr<WebSocket::Handler>()));
    connection.handleNewData();
}
