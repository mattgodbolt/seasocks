// Copyright (c) 2013-2015, Matt Godbolt
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

#include "seasocks/Request.h"
#include "seasocks/ResponseCode.h"

#include <map>
#include <memory>
#include <string>

namespace seasocks {

class ResponseWriter;

class Response {
public:
    virtual ~Response() = default;
    virtual void handle(std::shared_ptr<ResponseWriter> writer) = 0;

    // Called when the connection this response is being sent on is closed.
    virtual void cancel() = 0;

    static std::shared_ptr<Response> unhandled();

    static std::shared_ptr<Response> notFound();

    static std::shared_ptr<Response> error(ResponseCode code, const std::string& error);

    static std::shared_ptr<Response> textResponse(const std::string& response);
    static std::shared_ptr<Response> jsonResponse(const std::string& response);
    static std::shared_ptr<Response> htmlResponse(const std::string& response);
    static std::shared_ptr<Response> fileResponse(const Request &request, const std::string &filePath, const std::string &contentType, bool allowCompression, bool allowCaching);
};

}
