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

#include "seasocks/ResponseCode.h"

#include <map>
#include <memory>
#include <string>

namespace seasocks {

class Response {
public:
    virtual ~Response() {}
    virtual ResponseCode responseCode() const = 0;

    virtual const char* payload() const = 0;
    virtual size_t payloadSize() const = 0;

    virtual std::string contentType() const = 0;

    virtual bool keepConnectionAlive() const = 0;

    typedef std::multimap<std::string, std::string> Headers;
    virtual Headers getAdditionalHeaders() const = 0;

    static std::shared_ptr<Response> unhandled();

    static std::shared_ptr<Response> notFound();

    static std::shared_ptr<Response> error(ResponseCode code, const std::string& error);

    static std::shared_ptr<Response> textResponse(const std::string& response);
    static std::shared_ptr<Response> jsonResponse(const std::string& response);
    static std::shared_ptr<Response> htmlResponse(const std::string& response);
};

}
