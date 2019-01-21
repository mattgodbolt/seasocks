// Copyright (c) 2013-2017, Martin Charles
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

#include "seasocks/Response.h"
#include "seasocks/ResponseWriter.h"
#include "seasocks/TransferEncoding.h"

#include <iosfwd>

namespace seasocks {

// A low level way to write create responses. This class doesn't meddle with or
// assume anything based on the headers.
class StreamingResponse : public Response {
    bool closed = false;

public:
    virtual ~StreamingResponse() = default;
    virtual void handle(std::shared_ptr<ResponseWriter> writer) override;
    virtual void cancel() override;

    virtual std::shared_ptr<std::istream> getStream() const = 0;

    typedef std::multimap<std::string, std::string> Headers;
    virtual Headers getHeaders() const = 0;

    virtual ResponseCode responseCode() const = 0;

    virtual bool keepConnectionAlive() const = 0;
    virtual bool flushInstantly() const = 0;

    virtual size_t getBufferSize() const = 0;
    virtual TransferEncoding transferEncoding() const = 0;
};

}
