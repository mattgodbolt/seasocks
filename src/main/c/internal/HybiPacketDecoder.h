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

#pragma once

#include "seasocks/Logger.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace seasocks {

class HybiPacketDecoder {
    Logger& _logger;
    const std::vector<uint8_t>& _buffer;
    size_t _messageStart;

public:
    HybiPacketDecoder(Logger& logger, const std::vector<uint8_t>& buffer);

    enum class Opcode : uint8_t {
        Cont = 0x0, // Deprecated in latest hybi spec, here anyway.
        Text = 0x1,
        Binary = 0x2,
        Close = 0x8,
        Ping = 0x9,
        Pong = 0xA,
    };

    enum class MessageState {
        NoMessage,
        TextMessage,
        BinaryMessage,
        Error,
        Ping,
        Pong,
        Close
    };
    MessageState decodeNextMessage(std::vector<uint8_t>& messageOut, bool& deflateNeeded);
    MessageState decodeNextMessage(std::vector<uint8_t>& messageOut) {
        bool ignore;
        return decodeNextMessage(messageOut, ignore);
    }

    size_t numBytesDecoded() const;
};

}
