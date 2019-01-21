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

#include "internal/HybiAccept.h"
#include "internal/HybiPacketDecoder.h"

#include "seasocks/IgnoringLogger.h"

#include <catch2/catch.hpp>

#include <iostream>
#include <sstream>
#include <cstring>
#include <string>

using namespace seasocks;

static IgnoringLogger ignore;

void testSingleString(
    HybiPacketDecoder::MessageState expectedState,
    const char* expectedPayload,
    const std::vector<uint8_t>& v,
    uint32_t size = 0) {
    HybiPacketDecoder decoder(ignore, v);
    std::vector<uint8_t> decoded;
    CHECK(decoder.decodeNextMessage(decoded) == expectedState);
    CHECK(std::string(reinterpret_cast<const char*>(&decoded[0]), decoded.size()) == expectedPayload);
    CHECK(decoder.decodeNextMessage(decoded) == HybiPacketDecoder::MessageState::NoMessage);
    CHECK(decoder.numBytesDecoded() == (size ? size : v.size()));
}

void testLongString(size_t size, std::vector<uint8_t> v) {
    for (size_t i = 0; i < size; ++i) {
        v.push_back('A');
    }
    HybiPacketDecoder decoder(ignore, v);
    std::vector<uint8_t> decoded;
    CHECK(decoder.decodeNextMessage(decoded) == HybiPacketDecoder::MessageState::TextMessage);
    REQUIRE(decoded.size() == size);
    for (size_t i = 0; i < size; ++i) {
        REQUIRE(decoded[i] == 'A');
    }
    CHECK(decoder.decodeNextMessage(decoded) == HybiPacketDecoder::MessageState::NoMessage);
    CHECK(decoder.numBytesDecoded() == v.size());
}

TEST_CASE("textExamples", "[HybiTests]") {
    // CF. http://tools.ietf.org/html/draft-ietf-hybi-thewebsocketprotocol-10 #4.7
    testSingleString(HybiPacketDecoder::MessageState::TextMessage, "Hello", {0x81, 0x05, 0x48, 0x65, 0x6c, 0x6c, 0x6f});
    testSingleString(HybiPacketDecoder::MessageState::TextMessage, "Hello", {0x81, 0x85, 0x37, 0xfa, 0x21, 0x3d, 0x7f, 0x9f, 0x4d, 0x51, 0x58});
    testSingleString(HybiPacketDecoder::MessageState::Ping, "Hello", {0x89, 0x05, 0x48, 0x65, 0x6c, 0x6c, 0x6f});
}

TEST_CASE("withPartialMessageFollowing", "[HybiTests]") {
    testSingleString(HybiPacketDecoder::MessageState::TextMessage, "Hello", {0x81, 0x05, 0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x81}, 7);
    testSingleString(HybiPacketDecoder::MessageState::TextMessage, "Hello", {0x81, 0x05, 0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x81, 0x05}, 7);
    testSingleString(HybiPacketDecoder::MessageState::TextMessage, "Hello", {0x81, 0x05, 0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x81, 0x05, 0x48}, 7);
    testSingleString(HybiPacketDecoder::MessageState::TextMessage, "Hello", {0x81, 0x05, 0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x81, 0x05, 0x48, 0x65, 0x6c, 0x6c}, 7);
}

TEST_CASE("binaryMessage", "[HybiTests]") {
    std::vector<uint8_t> packet{0x82, 0x03, 0x00, 0x01, 0x02};
    std::vector<uint8_t> expected_body{0x00, 0x01, 0x02};
    HybiPacketDecoder decoder(ignore, packet);
    std::vector<uint8_t> decoded;
    CHECK(decoder.decodeNextMessage(decoded) == HybiPacketDecoder::MessageState::BinaryMessage);
    CHECK(decoded == expected_body);
    CHECK(decoder.decodeNextMessage(decoded) == HybiPacketDecoder::MessageState::NoMessage);
    CHECK(decoder.numBytesDecoded() == packet.size());
}

TEST_CASE("withTwoMessages", "[HybiTests]") {
    std::vector<uint8_t> data{
        0x81, 0x05, 0x48, 0x65, 0x6c, 0x6c, 0x6f,
        0x81, 0x07, 0x47, 0x6f, 0x6f, 0x64, 0x62, 0x79, 0x65};
    HybiPacketDecoder decoder(ignore, data);
    std::vector<uint8_t> decoded;
    CHECK(decoder.decodeNextMessage(decoded) == HybiPacketDecoder::MessageState::TextMessage);
    CHECK(std::string(reinterpret_cast<const char*>(&decoded[0]), decoded.size()) == "Hello");
    CHECK(decoder.decodeNextMessage(decoded) == HybiPacketDecoder::MessageState::TextMessage);
    CHECK(std::string(reinterpret_cast<const char*>(&decoded[0]), decoded.size()) == "Goodbye");
    CHECK(decoder.decodeNextMessage(decoded) == HybiPacketDecoder::MessageState::NoMessage);
    CHECK(decoder.numBytesDecoded() == data.size());
}

TEST_CASE("withTwoMessagesOneBeingMaskedd", "[HybiTests]") {
    std::vector<uint8_t> data{
        0x81, 0x05, 0x48, 0x65, 0x6c, 0x6c, 0x6f,                        // hello
        0x81, 0x85, 0x37, 0xfa, 0x21, 0x3d, 0x7f, 0x9f, 0x4d, 0x51, 0x58 // also hello
    };
    HybiPacketDecoder decoder(ignore, data);
    std::vector<uint8_t> decoded;
    CHECK(decoder.decodeNextMessage(decoded) == HybiPacketDecoder::MessageState::TextMessage);
    CHECK(std::string(reinterpret_cast<const char*>(&decoded[0]), decoded.size()) == "Hello");
    CHECK(decoder.decodeNextMessage(decoded) == HybiPacketDecoder::MessageState::TextMessage);
    CHECK(std::string(reinterpret_cast<const char*>(&decoded[0]), decoded.size()) == "Hello");
    CHECK(decoder.decodeNextMessage(decoded) == HybiPacketDecoder::MessageState::NoMessage);
    CHECK(decoder.numBytesDecoded() == data.size());
}

TEST_CASE("regressionBug", "[HybiTests]") {
    // top bit set of second byte of message used to trigger a MASK decode of the remainder
    std::vector<uint8_t> data{
        0x82, 0x05, 0x80, 0x81, 0x82, 0x83, 0x84};
    std::vector<uint8_t> expected_body{0x80, 0x81, 0x82, 0x83, 0x84};
    HybiPacketDecoder decoder(ignore, data);
    std::vector<uint8_t> decoded;
    CHECK(decoder.decodeNextMessage(decoded) == HybiPacketDecoder::MessageState::BinaryMessage);
    CHECK(decoded == expected_body);
    CHECK(decoder.numBytesDecoded() == data.size());
}

TEST_CASE("longStringExamples", "[HybiTests]") {
    // These are the binary examples, but cast as strings.
    testLongString(256, {0x81, 0x7E, 0x01, 0x00});
    testLongString(65536, {0x81, 0x7F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00});
}

TEST_CASE("accept", "[HybiTests]") {
    CHECK(getAcceptKey("dGhlIHNhbXBsZSBub25jZQ==") == "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=");
}

TEST_CASE("pings and pongs", "[HybiTests]") {
    testSingleString(HybiPacketDecoder::MessageState::Ping, "Hello", {0x89, 0x05, 0x48, 0x65, 0x6c, 0x6c, 0x6f});
    testSingleString(HybiPacketDecoder::MessageState::Pong, "Hello", {0x8a, 0x05, 0x48, 0x65, 0x6c, 0x6c, 0x6f});
}
