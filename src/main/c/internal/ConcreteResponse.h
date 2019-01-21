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

#include "seasocks/SynchronousResponse.h"

namespace seasocks {

class ConcreteResponse : public SynchronousResponse {
    ResponseCode _responseCode;
    const std::string _payload;
    const std::string _contentType;
    const Headers _headers;
    const bool _keepAlive;

public:
    ConcreteResponse(ResponseCode responseCode, const std::string& payload,
                     const std::string& contentType, const Headers& headers, bool keepAlive)
            : _responseCode(responseCode), _payload(payload), _contentType(contentType),
              _headers(headers), _keepAlive(keepAlive) {
    }

    virtual ResponseCode responseCode() const override {
        return _responseCode;
    }

    virtual const char* payload() const override {
        return _payload.c_str();
    }

    virtual size_t payloadSize() const override {
        return _payload.size();
    }

    virtual bool keepConnectionAlive() const override {
        return _keepAlive;
    }

    virtual std::string contentType() const override {
        return _contentType;
    }

    virtual Headers getAdditionalHeaders() const override {
        return _headers;
    }
};

}
