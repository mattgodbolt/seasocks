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

#include <stdexcept>
#include <unordered_map>

namespace seasocks {

class MockServerImpl : public ServerImpl {
public:
    virtual ~MockServerImpl() = default;

    std::string staticPath;
    std::unordered_map<std::string, std::shared_ptr<WebSocket::Handler>> handlers;

    void remove(Connection* /*connection*/) override {
    }
    bool subscribeToWriteEvents(Connection* /*connection*/) override {
        return false;
    };
    bool unsubscribeFromWriteEvents(Connection* /*connection*/) override {
        return false;
    }
    const std::string& getStaticPath() const override {
        return staticPath;
    }
    std::shared_ptr<WebSocket::Handler> getWebSocketHandler(const char* endpoint) const override {
        auto it = handlers.find(endpoint);
        if (it == handlers.end())
            return std::shared_ptr<WebSocket::Handler>();
        return it->second;
    }
    bool isCrossOriginAllowed(const std::string& /*endpoint*/) const override {
        return false;
    }
    std::shared_ptr<Response> handle(const Request& /*request*/) override {
        return std::shared_ptr<Response>();
    }
    std::string getStatsDocument() const override {
        return "";
    }
    void checkThread() const override {
    }
    Server& server() override {
        throw std::runtime_error("not supported");
    };
    size_t clientBufferSize() const override {
        return 512 * 1024;
    }
};

}
