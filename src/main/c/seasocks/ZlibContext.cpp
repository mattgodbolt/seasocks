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

#include "seasocks/ZlibContext.h"

#include <memory>
#include <stdexcept>

#include <zlib.h>

namespace seasocks {

struct ZlibContext::Impl {
    z_stream deflateStream;
    z_stream inflateStream;
    bool streamsInitialised = false;
    uint8_t buffer[16384];

    Impl(int deflateBits, int inflateBits, int memLevel) {
        int ret;

        deflateStream.zalloc = Z_NULL;
        deflateStream.zfree = Z_NULL;
        deflateStream.opaque = Z_NULL;

        ret = ::deflateInit2(
            &deflateStream,
            Z_DEFAULT_COMPRESSION,
            Z_DEFLATED,
            deflateBits * -1,
            memLevel,
            Z_DEFAULT_STRATEGY);

        if (ret != Z_OK) {
            throw std::runtime_error("error initialising zlib deflater");
        }

        inflateStream.zalloc = Z_NULL;
        inflateStream.zfree = Z_NULL;
        inflateStream.opaque = Z_NULL;
        inflateStream.avail_in = 0;
        inflateStream.next_in = Z_NULL;

        ret = ::inflateInit2(
            &inflateStream,
            inflateBits * -1);

        if (ret != Z_OK) {
            ::deflateEnd(&deflateStream);
            throw std::runtime_error("error initialising zlib inflater");
        }

        streamsInitialised = true;
    }

    ~Impl() {
        if (!streamsInitialised)
            return;
        ::deflateEnd(&deflateStream);
        ::inflateEnd(&inflateStream);
    }

    void deflate(const uint8_t* input, size_t inputLen, std::vector<uint8_t>& output) {
        deflateStream.next_in = (unsigned char*) input;
        deflateStream.avail_in = inputLen;

        if (inputLen > 0) {
            do {
                deflateStream.next_out = buffer;
                deflateStream.avail_out = sizeof(buffer);

                int ret = ::deflate(&deflateStream, Z_SYNC_FLUSH);

                if (ret != Z_OK && ret != Z_BUF_ERROR) {
                    throw std::runtime_error("error deflating message");
                }

                output.insert(output.end(), buffer, buffer + sizeof(buffer) - deflateStream.avail_out);
            } while (deflateStream.avail_out == 0);
        }

        // Add an empty uncompressed block if not present, then
        // remove 4-byte tail end prior to transmission (see RFC 7692, section 7.2.1)
        uint8_t tail_end[4] = {0x00, 0x00, 0xff, 0xff};
        if (output.size() < 5 || !std::equal(output.end() - 4, output.end(), tail_end)) {
            output.push_back(0x00);
        } else {
            output.resize(output.size() - 4);
        }
    }

    bool inflate(std::vector<uint8_t>& input, std::vector<uint8_t>& output, int& zlibError) {
        // Append 4 octets prior to decompression (see RFC 7692, section 7.2.2)
        uint8_t tail_end[4] = {0x00, 0x00, 0xff, 0xff};
        input.insert(input.end(), tail_end, tail_end + 4);

        inflateStream.next_in = input.data();
        inflateStream.avail_in = input.size();

        do {
            inflateStream.next_out = buffer;
            inflateStream.avail_out = sizeof(buffer);

            int ret = ::inflate(&inflateStream, Z_SYNC_FLUSH);

            if (ret != Z_OK && ret != Z_STREAM_END && ret != Z_BUF_ERROR) {
                zlibError = ret;
                return false;
            }

            output.insert(output.end(), buffer, buffer + sizeof(buffer) - inflateStream.avail_out);
        } while (inflateStream.avail_out == 0);

        return true;
    }
};

ZlibContext::ZlibContext() = default;

ZlibContext::~ZlibContext() = default;

void ZlibContext::initialise(int deflateBits, int inflateBits, int memLevel) {
    _impl = std::make_unique<Impl>(deflateBits, inflateBits, memLevel);
}

void ZlibContext::deflate(const uint8_t* input, size_t inputLen, std::vector<uint8_t>& output) {
    return _impl->deflate(input, inputLen, output);
}

bool ZlibContext::inflate(std::vector<uint8_t>& input, std::vector<uint8_t>& output, int& zlibError) {
    return _impl->inflate(input, output, zlibError);
}

}
