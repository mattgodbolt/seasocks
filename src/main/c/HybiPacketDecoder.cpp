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

#include "internal/HybiPacketDecoder.h"
#include "internal/LogStream.h"

#include <arpa/inet.h>
#include <byteswap.h>
#include <cstring>

namespace seasocks {

HybiPacketDecoder::HybiPacketDecoder(Logger& logger,
                                     const std::vector<uint8_t>& buffer)
        : _logger(logger),
          _buffer(buffer),
          _messageStart(0) {
}

HybiPacketDecoder::MessageState HybiPacketDecoder::decodeNextMessage(
    std::vector<uint8_t>& messageOut, bool& deflateNeeded) {
    if (_messageStart + 1 >= _buffer.size()) {
        return MessageState::NoMessage;
    }
    if ((_buffer[_messageStart] & 0x80) == 0) {
        // FIN bit is not clear...
        // TODO: support
        LS_WARNING(&_logger, "Received hybi frame without FIN bit set - unsupported");
        return MessageState::Error;
    }

    auto reservedBits = _buffer[_messageStart] & (7 << 4);
    if ((reservedBits & 0x30) != 0) {
        LS_WARNING(&_logger, "Received hybi frame with reserved bits set - error");
        return MessageState::Error;
    }

    deflateNeeded = !!(reservedBits & 0x40);

    auto opcode = static_cast<Opcode>(_buffer[_messageStart] & 0xf);
    size_t payloadLength = _buffer[_messageStart + 1] & 0x7fu;
    auto maskBit = _buffer[_messageStart + 1] & 0x80;
    auto ptr = _messageStart + 2;
    if (payloadLength == 126) {
        if (_buffer.size() < 4) {
            return MessageState::NoMessage;
        }
        uint16_t raw_length;
        memcpy(&raw_length, &_buffer[ptr], sizeof(raw_length));
        payloadLength = htons(raw_length);
        ptr += 2;
    } else if (payloadLength == 127) {
        if (_buffer.size() < 10) {
            return MessageState::NoMessage;
        }
        uint64_t raw_length;
        memcpy(&raw_length, &_buffer[ptr], sizeof(raw_length));
        payloadLength = __bswap_64(raw_length);
        ptr += 8;
    }
    uint32_t mask = 0;
    if (maskBit) {
        // MASK is set.
        if (_buffer.size() < ptr + 4) {
            return MessageState::NoMessage;
        }
        uint32_t raw_length;
        memcpy(&raw_length, &_buffer[ptr], sizeof(raw_length));
        mask = htonl(raw_length);
        ptr += 4;
    }
    auto bytesLeftInBuffer = _buffer.size() - ptr;
    if (payloadLength > bytesLeftInBuffer) {
        return MessageState::NoMessage;
    }

    messageOut.clear();
    messageOut.reserve(payloadLength);
    for (auto i = 0u; i < payloadLength; ++i) {
        auto byteShift = (3 - (i & 3)) * 8;
        messageOut.push_back(static_cast<uint8_t>((_buffer[ptr++] ^ (mask >> byteShift)) & 0xff));
    }
    _messageStart = ptr;
    switch (opcode) {
        default:
            LS_WARNING(&_logger, "Received hybi frame with unknown opcode "
                                     << static_cast<int>(opcode));
            return MessageState::Error;
        case Opcode::Text:
            return MessageState::TextMessage;
        case Opcode::Binary:
            return MessageState::BinaryMessage;
        case Opcode::Ping:
            return MessageState::Ping;
        case Opcode::Pong:
            return MessageState::Pong;
        case Opcode::Close:
            return MessageState::Close;
    }
}

size_t HybiPacketDecoder::numBytesDecoded() const {
    return _messageStart;
}

}
