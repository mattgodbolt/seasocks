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

#include "seasocks/StreamingResponse.h"
#include "seasocks/ToString.h"
#include "seasocks/StringUtil.h"

using namespace seasocks;

void StreamingResponse::handle(std::shared_ptr<ResponseWriter> writer) {
    writer->begin(responseCode(), transferEncoding());

    auto headers = getHeaders();
    for (auto& header : headers) {
        writer->header(header.first, header.second);
    }

    std::shared_ptr<std::istream> stream = getStream();

    auto bufSize = getBufferSize();
    bool flush = flushInstantly();
    std::unique_ptr<char[]> buffer(new char[bufSize]);

    while (!closed) {
        // blocks until buffer is full or eof is reached
        stream->read(buffer.get(), bufSize);

        bool isEof = stream->eof();
        bool isGood = stream->good();
        if (isGood || isEof) {
            // everything is fine, push data to client
            writer->payload(buffer.get(), stream->gcount(), flush);
        }

        if (!isGood) {
            // EOF or error occured
            // ignore the error since we can't access it
            closed = true;
        }
    };

    writer->finish(keepConnectionAlive());
}

void StreamingResponse::cancel() {
    closed = true;
}
