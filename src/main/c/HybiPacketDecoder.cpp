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
#include <cstring>

// https://tools.ietf.org/html/rfc6455#page-27

/*
	0                   1                   2                   3
	0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
	+-+-+-+-+-------+-+-------------+-------------------------------+
	|F|R|R|R| opcode|M| Payload len |    Extended payload length    |
	|I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
	|N|V|V|V|       |S|             |   (if payload len==126/127)   |
	| |1|2|3|       |K|             |                               |
	+-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
	|     Extended payload length continued, if payload len == 127  |
	+ - - - - - - - - - - - - - - - +-------------------------------+
	|                               |Masking-key, if MASK set to 1  |
	+-------------------------------+-------------------------------+
	| Masking-key (continued)       |          Payload Data         |
	+-------------------------------- - - - - - - - - - - - - - - - +
	:                     Payload Data continued ...                :
	+ - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
	|                     Payload Data continued ...                |
	+---------------------------------------------------------------+
*/

using namespace std;

namespace seasocks {
	
	HybiPacketDecoder::HybiPacketDecoder(Logger& logger, const std::vector<uint8_t>& buffer) :_logger(logger),_buffer(buffer),_messageStart(0) {
	}

	HybiPacketDecoder::MessageState HybiPacketDecoder::decodeNextMessage(vector<uint8_t>& messageOut, bool& deflateNeeded, Opcode& firstOpcodeFinunset) {
		if (_messageStart + 1 >= _buffer.size()) {
			return MessageState::NoMessage;
		}

		bool finset = (_buffer[_messageStart] & 0x80) == 0x80;

		auto reservedBits = _buffer[_messageStart] & (7 << 4);
		if ((reservedBits & 0x30) != 0) {
			LS_WARNING(&_logger, "Received hybi frame with reserved bits set - error");
			return MessageState::Error;
		}

		deflateNeeded = !!(reservedBits & 0x40);

		auto opcode = static_cast<Opcode>(_buffer[_messageStart] & 0xf);
		if (!finset && opcode != Opcode::Cont) {
			firstOpcodeFinunset = opcode;
		}		

		size_t payloadLength = _buffer[_messageStart + 1] & 0x7fu;
		auto maskBit = _buffer[_messageStart + 1] & 0x80;
		auto ptr = _messageStart + 2;
		if (payloadLength == 126) {
			if (_buffer.size() < 4) { return MessageState::NoMessage; }
			uint16_t raw_length;
			memcpy(&raw_length, &_buffer[ptr], sizeof(raw_length));
			payloadLength = htons(raw_length);
			ptr += 2;
		}
		else if (payloadLength == 127) {
			if (_buffer.size() < 10) { return MessageState::NoMessage; }
			uint64_t raw_length;
			memcpy(&raw_length, &_buffer[ptr], sizeof(raw_length));
			payloadLength = (size_t)__bswap_64(raw_length);
			ptr += 8;
		}
		uint32_t mask = 0;
		if (maskBit) {
			// MASK is set.
			if (_buffer.size() < ptr + 4) { return MessageState::NoMessage; }
			uint32_t raw_length;
			memcpy(&raw_length, &_buffer[ptr], sizeof(raw_length));
			mask = htonl(raw_length);
			ptr += 4;
		}
		else
		{
			return MessageState::Error;
		}
		auto bytesLeftInBuffer = _buffer.size() - ptr;
		if (payloadLength > bytesLeftInBuffer) { return MessageState::NoMessage; }

		messageOut.clear();
		messageOut.reserve(payloadLength);
		for (auto i = 0u; i < payloadLength; ++i) {
			auto byteShift = (3 - (i & 3)) * 8;
			messageOut.push_back(static_cast<uint8_t>((_buffer[ptr++] ^ (mask >> byteShift)) & 0xff));
		}
		_messageStart = ptr;

		switch (opcode) {
		case Opcode::Cont:
			switch (firstOpcodeFinunset)
			{
			case Opcode::Text:
				if (finset) {
					return MessageState::TextMessage;
				}
				else{
					return MessageState::TextMessageFragment;
				}				
			case Opcode::Binary:
				if (finset) {
					return MessageState::BinaryMessage;
				}
				else {
					return MessageState::BinaryMessageFragment;
				}				
			default:
				if (finset) {
					LS_WARNING(&_logger, "Received hybi frame with fin set and first fragment opcoce is not text or binary ");
				}
				else {
					LS_WARNING(&_logger, "Received hybi frame with fin unset and first fragment opcoce is not text or binary ");
				}				
				return MessageState::Error;
			}			
			break;
		case Opcode::Text:
			if (!finset) {
				return MessageState::TextMessageFragment;
			}
			else {
				return MessageState::TextMessage;
			}			
		case Opcode::Binary:
			if (!finset) {
				return MessageState::BinaryMessageFragment;
			}
			else {
				return MessageState::BinaryMessage;
			}
			
		case Opcode::Ping:
			return MessageState::Ping;
		case Opcode::Pong:
			return MessageState::Pong;
		case Opcode::Close:
			return MessageState::Close;
		default:
			LS_WARNING(&_logger, "Received hybi frame with unknown opcode "
				<< static_cast<int>(opcode));
			return MessageState::Error;
		}

	}

	size_t HybiPacketDecoder::numBytesDecoded() const {
		return _messageStart;
	}

}
