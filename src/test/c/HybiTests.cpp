#include <gmock/gmock.h>

#include <string>
#include "internal/HybiAccept.h"
#include "internal/HybiPacketDecoder.h"
#include "seasocks/ignoringlogger.h"
#include <iostream>
#include <sstream>
#include <string.h>

using namespace SeaSocks;

static IgnoringLogger ignore;

void testSingleString(
		HybiPacketDecoder::MessageState expectedState,
		const char* expectedPayload,
		const std::vector<uint8_t>& v,
		uint32_t size = 0) {
	HybiPacketDecoder decoder(ignore, v);
	std::string decoded;
	ASSERT_EQ(expectedState, decoder.decodeNextMessage(decoded));
	ASSERT_EQ(expectedPayload, decoded);
	ASSERT_EQ(HybiPacketDecoder::NoMessage, decoder.decodeNextMessage(decoded));
	ASSERT_EQ(size ? size : v.size(), decoder.numBytesDecoded());
}

void testLongString(size_t size, std::vector<uint8_t> v) {
	for (size_t i = 0; i < size; ++i) {
		v.push_back('A');
	}
	HybiPacketDecoder decoder(ignore, v);
	std::string decoded;
	ASSERT_EQ(HybiPacketDecoder::Message, decoder.decodeNextMessage(decoded));
	ASSERT_EQ(size, decoded.size());
	for (size_t i = 0; i < size; ++i) {
		ASSERT_EQ('A', decoded[i]);
	}
	ASSERT_EQ(HybiPacketDecoder::NoMessage, decoder.decodeNextMessage(decoded));
	ASSERT_EQ(v.size(), decoder.numBytesDecoded());
}

void testTextExamples() {
	// CF. http://tools.ietf.org/html/draft-ietf-hybi-thewebsocketprotocol-10 #4.7
	testSingleString(HybiPacketDecoder::Message, "Hello", {0x81, 0x05, 0x48, 0x65, 0x6c, 0x6c, 0x6f});
	testSingleString(HybiPacketDecoder::Message, "Hello", {0x81, 0x85, 0x37, 0xfa, 0x21, 0x3d, 0x7f, 0x9f, 0x4d, 0x51, 0x58});
	testSingleString(HybiPacketDecoder::Ping, "Hello", {0x89, 0x05, 0x48, 0x65, 0x6c, 0x6c, 0x6f});
}

void testWithPartialMessageFollowing() {
	testSingleString(HybiPacketDecoder::Message, "Hello", {0x81, 0x05, 0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x81}, 7);
	testSingleString(HybiPacketDecoder::Message, "Hello", {0x81, 0x05, 0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x81, 0x05}, 7);
	testSingleString(HybiPacketDecoder::Message, "Hello", {0x81, 0x05, 0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x81, 0x05, 0x48}, 7);
	testSingleString(HybiPacketDecoder::Message, "Hello", {0x81, 0x05, 0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x81, 0x05, 0x48, 0x65, 0x6c, 0x6c}, 7);
}

void testWithTwoMessages() {
	std::vector<uint8_t> data {
		0x81, 0x05, 0x48, 0x65, 0x6c, 0x6c, 0x6f,
		0x81, 0x07, 0x47, 0x6f, 0x6f, 0x64, 0x62, 0x79, 0x65};
	HybiPacketDecoder decoder(ignore, data);
	std::string decoded;
	ASSERT_EQ(HybiPacketDecoder::Message, decoder.decodeNextMessage(decoded));
	ASSERT_EQ("Hello", decoded);
	ASSERT_EQ(HybiPacketDecoder::Message, decoder.decodeNextMessage(decoded));
	ASSERT_EQ("Goodbye", decoded);
	ASSERT_EQ(HybiPacketDecoder::NoMessage, decoder.decodeNextMessage(decoded));
	ASSERT_EQ(data.size(), decoder.numBytesDecoded());
}

void testLongStringExamples() {
	// These are the binary examples, but cast as strings.
	testLongString(256, {0x81, 0x7E, 0x01, 0x00});
	testLongString(65536, {0x81, 0x7F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00});
}

void testAccept() {
	ASSERT_EQ("s3pPLMBiTxaQ9kYGzzhZRbK+xOo=", getAcceptKey("dGhlIHNhbXBsZSBub25jZQ=="));
}
