#include "internal/HybiAccept.h"
#include "internal/HybiPacketDecoder.h"

#include "seasocks/IgnoringLogger.h"

#include <gmock/gmock.h>

#include <iostream>
#include <sstream>
#include <string.h>
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
	ASSERT_EQ(expectedState, decoder.decodeNextMessage(decoded));
	ASSERT_EQ(expectedPayload, std::string(reinterpret_cast<const char*>(&decoded[0]), decoded.size()));
	ASSERT_EQ(HybiPacketDecoder::NoMessage, decoder.decodeNextMessage(decoded));
	ASSERT_EQ(size ? size : v.size(), decoder.numBytesDecoded());
}

void testLongString(size_t size, std::vector<uint8_t> v) {
	for (size_t i = 0; i < size; ++i) {
		v.push_back('A');
	}
	HybiPacketDecoder decoder(ignore, v);
	std::vector<uint8_t> decoded;
	ASSERT_EQ(HybiPacketDecoder::TextMessage, decoder.decodeNextMessage(decoded));
	ASSERT_EQ(size, decoded.size());
	for (size_t i = 0; i < size; ++i) {
		ASSERT_EQ('A', decoded[i]);
	}
	ASSERT_EQ(HybiPacketDecoder::NoMessage, decoder.decodeNextMessage(decoded));
	ASSERT_EQ(v.size(), decoder.numBytesDecoded());
}

TEST(HybiTests, textExamples) {
	// CF. http://tools.ietf.org/html/draft-ietf-hybi-thewebsocketprotocol-10 #4.7
	testSingleString(HybiPacketDecoder::TextMessage, "Hello", {0x81, 0x05, 0x48, 0x65, 0x6c, 0x6c, 0x6f});
	testSingleString(HybiPacketDecoder::TextMessage, "Hello", {0x81, 0x85, 0x37, 0xfa, 0x21, 0x3d, 0x7f, 0x9f, 0x4d, 0x51, 0x58});
	testSingleString(HybiPacketDecoder::Ping, "Hello", {0x89, 0x05, 0x48, 0x65, 0x6c, 0x6c, 0x6f});
}

TEST(HybiTests, withPartialMessageFollowing) {
	testSingleString(HybiPacketDecoder::TextMessage, "Hello", {0x81, 0x05, 0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x81}, 7);
	testSingleString(HybiPacketDecoder::TextMessage, "Hello", {0x81, 0x05, 0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x81, 0x05}, 7);
	testSingleString(HybiPacketDecoder::TextMessage, "Hello", {0x81, 0x05, 0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x81, 0x05, 0x48}, 7);
	testSingleString(HybiPacketDecoder::TextMessage, "Hello", {0x81, 0x05, 0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x81, 0x05, 0x48, 0x65, 0x6c, 0x6c}, 7);
}

TEST(HybiTests, binaryMessage) {
	std::vector<uint8_t> packet { 0x82, 0x03, 0x00, 0x01, 0x02 };
	std::vector<uint8_t> expected_body { 0x00, 0x01, 0x02 };
	HybiPacketDecoder decoder(ignore, packet);
	std::vector<uint8_t> decoded;
	ASSERT_EQ(HybiPacketDecoder::BinaryMessage, decoder.decodeNextMessage(decoded));
	ASSERT_EQ(expected_body, decoded);
	ASSERT_EQ(HybiPacketDecoder::NoMessage, decoder.decodeNextMessage(decoded));
	ASSERT_EQ(packet.size(), decoder.numBytesDecoded());
}

TEST(HybiTests, withTwoMessages) {
	std::vector<uint8_t> data {
		0x81, 0x05, 0x48, 0x65, 0x6c, 0x6c, 0x6f,
		0x81, 0x07, 0x47, 0x6f, 0x6f, 0x64, 0x62, 0x79, 0x65};
	HybiPacketDecoder decoder(ignore, data);
	std::vector<uint8_t> decoded;
	ASSERT_EQ(HybiPacketDecoder::TextMessage, decoder.decodeNextMessage(decoded));
	ASSERT_EQ("Hello", std::string(reinterpret_cast<const char*>(&decoded[0]), decoded.size()));
	ASSERT_EQ(HybiPacketDecoder::TextMessage, decoder.decodeNextMessage(decoded));
	ASSERT_EQ("Goodbye", std::string(reinterpret_cast<const char*>(&decoded[0]), decoded.size()));
	ASSERT_EQ(HybiPacketDecoder::NoMessage, decoder.decodeNextMessage(decoded));
	ASSERT_EQ(data.size(), decoder.numBytesDecoded());
}

TEST(HybiTests, withTwoMessagesOneBeingMaskedd) {
	std::vector<uint8_t> data {
		0x81, 0x05, 0x48, 0x65, 0x6c, 0x6c, 0x6f,  // hello
		0x81, 0x85, 0x37, 0xfa, 0x21, 0x3d, 0x7f, 0x9f, 0x4d, 0x51, 0x58  // also hello
	};
	HybiPacketDecoder decoder(ignore, data);
	std::vector<uint8_t> decoded;
	ASSERT_EQ(HybiPacketDecoder::TextMessage, decoder.decodeNextMessage(decoded));
	ASSERT_EQ("Hello", std::string(reinterpret_cast<const char*>(&decoded[0]), decoded.size()));
	ASSERT_EQ(HybiPacketDecoder::TextMessage, decoder.decodeNextMessage(decoded));
	ASSERT_EQ("Hello", std::string(reinterpret_cast<const char*>(&decoded[0]), decoded.size()));
	ASSERT_EQ(HybiPacketDecoder::NoMessage, decoder.decodeNextMessage(decoded));
	ASSERT_EQ(data.size(), decoder.numBytesDecoded());

}

TEST(HybiTests, longStringExamples) {
	// These are the binary examples, but cast as strings.
	testLongString(256, {0x81, 0x7E, 0x01, 0x00});
	testLongString(65536, {0x81, 0x7F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00});
}

TEST(HybiTests, accept) {
	ASSERT_EQ("s3pPLMBiTxaQ9kYGzzhZRbK+xOo=", getAcceptKey("dGhlIHNhbXBsZSBub25jZQ=="));
}
