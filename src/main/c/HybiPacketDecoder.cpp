#include "internal/HybiPacketDecoder.h"
#include "internal/LogStream.h"

#include <arpa/inet.h>

namespace seasocks {

HybiPacketDecoder::HybiPacketDecoder(Logger& logger, const std::vector<uint8_t>& buffer) :
    _logger(logger),
    _buffer(buffer),
    _messageStart(0) {
}

HybiPacketDecoder::MessageState HybiPacketDecoder::decodeNextMessage(std::vector<uint8_t>& messageOut) {
    if (_messageStart + 1 >= _buffer.size()) {
        return NoMessage;
    }
    if ((_buffer[_messageStart] & 0x80) == 0) {
        // FIN bit is not clear...
        // TODO: support
        LS_WARNING(&_logger, "Received hybi frame without FIN bit set - unsupported");
        return Error;
    }
    if ((_buffer[_messageStart] & (7<<4)) != 0) {
        LS_WARNING(&_logger, "Received hybi frame with reserved bits set - error");
        return Error;
    }
    auto opcode = _buffer[_messageStart] & 0xf;
    size_t payloadLength = _buffer[_messageStart + 1] & 0x7f;
    auto ptr = _messageStart + 2;
    if (payloadLength == 126) {
        if (_buffer.size() < 4) { return NoMessage; }
        payloadLength = htons(*reinterpret_cast<const uint16_t*>(&_buffer[ptr]));
        ptr += 2;
    } else if (payloadLength == 127) {
        if (_buffer.size() < 10) { return NoMessage; }
        payloadLength = __bswap_64(*reinterpret_cast<const uint64_t*>(&_buffer[ptr]));
        ptr += 8;
    }
    uint32_t mask = 0;
    if (_buffer[ptr + 1] & 0x80) {
        // MASK is set.
        if (_buffer.size() < ptr + 4) { return NoMessage; }
        mask = htonl(*reinterpret_cast<const uint32_t*>(&_buffer[ptr]));
        ptr += 4;
    }
    auto bytesLeftInBuffer = _buffer.size() - ptr;
    if (payloadLength > bytesLeftInBuffer) { return NoMessage; }

    messageOut.clear();
    messageOut.reserve(payloadLength);
    for (auto i = 0u; i < payloadLength; ++i) {
        auto byteShift = (3 - (i & 3)) * 8;
        messageOut.push_back(static_cast<char>((_buffer[ptr++] ^ (mask >> byteShift)) & 0xff));
    }
    _messageStart = ptr;
    switch (opcode) {
    default:
        LS_WARNING(&_logger, "Received hybi frame with unknown opcode " << opcode);
        return Error;
    case OPCODE_TEXT:
        return TextMessage;
    case OPCODE_BINARY:
        return BinaryMessage;
    case OPCODE_PING:
        return Ping;
    case OPCODE_CLOSE:
        return Close;
    }
}

size_t HybiPacketDecoder::numBytesDecoded() const {
    return _messageStart;
}

}
