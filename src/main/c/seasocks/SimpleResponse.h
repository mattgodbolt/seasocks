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

#include "seasocks/StreamingResponse.h"

namespace seasocks {

class SimpleResponse : public StreamingResponse {
    ResponseCode _responseCode;
    std::shared_ptr<std::istream> _stream;
    const Headers _headers;
    const bool _keepAlive;
    const bool _flushInstantly;
    const size_t _bufferSize;
    const TransferEncoding _transferEncoding;

public:
    static constexpr size_t DefaultBufferSize = 16 * 1024 * 1024u;

    SimpleResponse(ResponseCode responseCode, std::shared_ptr<std::istream> stream,
                   const Headers& headers, bool keepAlive = true, bool flushInstantly = false,
                   size_t bufferSize = DefaultBufferSize,
                   TransferEncoding transferEncoding = TransferEncoding::Raw)
            : _responseCode(responseCode), _stream(stream), _headers(headers),
              _keepAlive(keepAlive), _flushInstantly(flushInstantly),
              _bufferSize(bufferSize), _transferEncoding(transferEncoding) {
    }

    virtual std::shared_ptr<std::istream> getStream() const override {
        return _stream;
    }

    virtual Headers getHeaders() const override {
        return _headers;
    }

    virtual ResponseCode responseCode() const override {
        return _responseCode;
    }

    virtual bool keepConnectionAlive() const override {
        return _keepAlive;
    }

    virtual bool flushInstantly() const override {
        return _flushInstantly;
    }

    virtual size_t getBufferSize() const override {
        return _bufferSize;
    }

    virtual TransferEncoding transferEncoding() const override {
        return _transferEncoding;
    }
};

}
