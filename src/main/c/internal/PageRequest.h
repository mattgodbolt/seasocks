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

#include "internal/HeaderMap.h"
#include "seasocks/Request.h"

#include <unordered_map>
#include <vector>

namespace seasocks {

class PageRequest : public Request {
    std::shared_ptr<Credentials> _credentials;
    const sockaddr_in _remoteAddress;
    const std::string _requestUri;
    Server& _server;
    const Verb _verb;
    std::vector<uint8_t> _content;
    HeaderMap _headers;
    const size_t _contentLength;

public:
    PageRequest(
        const sockaddr_in& remoteAddress,
        const std::string& requestUri,
        Server& server,
        Verb verb,
        HeaderMap&& headers);

    virtual Server& server() const override {
        return _server;
    }

    virtual Verb verb() const override {
        return _verb;
    }

    virtual std::shared_ptr<Credentials> credentials() const override {
        return _credentials;
    }

    virtual const sockaddr_in& getRemoteAddress() const override {
        return _remoteAddress;
    }

    virtual const std::string& getRequestUri() const override {
        return _requestUri;
    }

    virtual size_t contentLength() const override {
        return _contentLength;
    }

    virtual const uint8_t* content() const override {
        return _contentLength > 0 ? &_content[0] : nullptr;
    }

    virtual bool hasHeader(const std::string& name) const override {
        return _headers.find(name) != _headers.end();
    }

    virtual std::string getHeader(const std::string& name) const override {
        auto iter = _headers.find(name);
        return iter == _headers.end() ? std::string() : iter->second;
    }

    bool consumeContent(std::vector<uint8_t>& buffer);

    size_t getUintHeader(const std::string& name) const;
};

} // namespace seasocks
