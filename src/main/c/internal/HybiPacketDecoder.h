#pragma once

#include "seasocks/logger.h"

#include <memory>
#include <string>
#include <vector>

namespace SeaSocks {

class HybiPacketDecoder {
	Logger& _logger;
	const std::vector<uint8_t>& _buffer;
	size_t _messageStart;
public:
	HybiPacketDecoder(Logger& logger, const std::vector<uint8_t>& buffer);

	enum {
		OPCODE_CONT = 0x0,  // Deprecated in latest hybi spec, here anyway.
		OPCODE_TEXT = 0x1,
		OPCODE_BINARY = 0x2,
		OPCODE_CLOSE = 0x8,
		OPCODE_PING = 0x9,
		OPCODE_PONG = 0xA,
	};

	enum MessageState {
		NoMessage,
		Message,
		Error,
		Ping,
		Close
	};
	MessageState decodeNextMessage(std::string& messageOut);

	size_t numBytesDecoded() const;
};

}
