// Copyright (c) 2013-2026, Matt Godbolt and Nguyen Tran
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

#include "internal/AcceptEncoding.h"
#include "seasocks/ZlibContext.h"

#include <catch2/catch_test_macros.hpp>

#include <zlib.h>

#include <string>
#include <vector>

using namespace seasocks;

namespace {

// decompress a gzip stream (windowBits 15+16), to verify gzip() output round-trips
std::string gunzip(const std::vector<uint8_t>& input) {
    z_stream stream;
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;
    stream.next_in = const_cast<z_const Bytef*>(input.data());
    stream.avail_in = static_cast<uInt>(input.size());
    REQUIRE(::inflateInit2(&stream, 15 + 16) == Z_OK);

    std::string output;
    uint8_t buffer[16384];
    int ret;
    do {
        stream.next_out = buffer;
        stream.avail_out = sizeof(buffer);
        ret = ::inflate(&stream, Z_NO_FLUSH);
        REQUIRE((ret == Z_OK || ret == Z_STREAM_END));
        output.append(reinterpret_cast<char*>(buffer), sizeof(buffer) - stream.avail_out);
    } while (ret != Z_STREAM_END);

    ::inflateEnd(&stream);
    return output;
}

} // namespace

TEST_CASE("gzip helper round-trips", "[HttpCompressionTests]") {
    std::string original(2000, 'x');
    original += "some readable text to compress and check";
    auto in = reinterpret_cast<const uint8_t*>(original.data());

    ZlibContext context;
    std::vector<uint8_t> compressed;
    context.gzip(in, original.size(), compressed);

    // gzip stream starts with the magic bytes 1f 8b
    REQUIRE(compressed.size() >= 2);
    CHECK(compressed[0] == 0x1f);
    CHECK(compressed[1] == 0x8b);
    // a repetitive body must shrink
    CHECK(compressed.size() < original.size());
    // and it must decode back to exactly the input
    CHECK(gunzip(compressed) == original);
}

TEST_CASE("empty body gzips to a valid stream", "[HttpCompressionTests]") {
    ZlibContext context;
    std::vector<uint8_t> compressed;
    context.gzip(nullptr, 0, compressed);
    CHECK(compressed[0] == 0x1f);
    CHECK(compressed[1] == 0x8b);
    CHECK(gunzip(compressed).empty());
}

TEST_CASE("gzip reuses one context across calls", "[HttpCompressionTests]") {
    ZlibContext context;
    const std::string first = "the first response body, repeated repeated repeated";
    const std::string second = "an entirely different second body on the same context";
    std::vector<uint8_t> a;
    std::vector<uint8_t> b;
    context.gzip(reinterpret_cast<const uint8_t*>(first.data()), first.size(), a);
    context.gzip(reinterpret_cast<const uint8_t*>(second.data()), second.size(), b);
    CHECK(gunzip(a) == first);
    CHECK(gunzip(b) == second);
}

TEST_CASE("acceptsGzip honours the Accept-Encoding value", "[HttpCompressionTests]") {
    SECTION("gzip listed with a positive or default weight is acceptable") {
        CHECK(acceptsGzip("gzip"));
        CHECK(acceptsGzip("gzip, deflate, br"));
        CHECK(acceptsGzip("GZIP"));
        CHECK(acceptsGzip("gzip;q=0.5"));
        CHECK(acceptsGzip("gzip; q=0.001"));
        CHECK(acceptsGzip("br;q=1.0, gzip;q=0.8"));
    }
    SECTION("gzip with a zero weight is not acceptable") {
        CHECK_FALSE(acceptsGzip("gzip;q=0"));
        CHECK_FALSE(acceptsGzip("gzip;Q=0"));
        CHECK_FALSE(acceptsGzip("gzip; q=0"));
    }
    SECTION("a malformed weight falls back to acceptable") {
        CHECK(acceptsGzip("gzip;q="));
        CHECK(acceptsGzip("gzip;q=abc"));
        // stod parses these values without throwing; the finite-check keeps them lenient
        CHECK(acceptsGzip("gzip;q=nan"));
        CHECK(acceptsGzip("gzip;q=inf"));
    }
    SECTION("the wildcard applies only when gzip is not listed explicitly") {
        CHECK(acceptsGzip("br, *"));
        CHECK_FALSE(acceptsGzip("br, *;q=0"));
        CHECK(acceptsGzip("gzip, *;q=0"));
        CHECK_FALSE(acceptsGzip("gzip;q=0, *"));
    }
    SECTION("gzip absent without a usable wildcard is not acceptable") {
        CHECK_FALSE(acceptsGzip("br, deflate"));
        CHECK_FALSE(acceptsGzip(""));
    }
}
