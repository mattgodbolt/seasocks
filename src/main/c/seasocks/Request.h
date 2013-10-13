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

#pragma once

#include "seasocks/Credentials.h"

#include <netinet/in.h>

#include <cstdint>
#include <memory>

namespace seasocks {

class Request {
public:
    virtual ~Request() {}

    enum Verb {
        Invalid,
        WebSocket,
        Get,
        Put,
        Post,
        Delete,
    };

    virtual Verb verb() const = 0;

    static const char* name(Verb v);
    static Verb verb(const char *verb);

    /**
     * Returns the credentials associated with this request.
     */
    virtual std::shared_ptr<Credentials> credentials() const = 0;

    virtual const sockaddr_in& getRemoteAddress() const = 0;

    virtual const std::string& getRequestUri() const = 0;

    virtual size_t contentLength() const = 0;

    virtual const uint8_t* content() const = 0;

    virtual bool hasHeader(const std::string& name) const = 0;

    virtual std::string getHeader(const std::string& name) const = 0;
};

}  // namespace seasocks
