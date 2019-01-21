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

#include "seasocks/WebSocket.h"

#include <string>

namespace seasocks {

class Connection;
class Request;
class Response;
class Server;

// Internal implementation used to give access to internals to Connections.
class ServerImpl {
public:
    virtual ~ServerImpl() = default;

    virtual void remove(Connection* connection) = 0;
    virtual bool subscribeToWriteEvents(Connection* connection) = 0;
    virtual bool unsubscribeFromWriteEvents(Connection* connection) = 0;
    virtual const std::string& getStaticPath() const = 0;
    virtual std::shared_ptr<WebSocket::Handler> getWebSocketHandler(const char* endpoint) const = 0;
    virtual bool isCrossOriginAllowed(const std::string& endpoint) const = 0;
    virtual std::shared_ptr<Response> handle(const Request& request) = 0;
    virtual std::string getStatsDocument() const = 0;
    virtual void checkThread() const = 0;
    virtual Server& server() = 0;
    virtual size_t clientBufferSize() const = 0;
};

}
