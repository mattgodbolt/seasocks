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

#include "internal/Base64.h"

#include <cstdint>

namespace seasocks {
namespace {
const char cb64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
}

std::string base64Encode(const void* data, size_t length) {
    std::string output;
    const auto dataPtr = reinterpret_cast<const uint8_t*>(data);
    for (auto i = 0u; i < length; i += 3) {
        const auto bytesLeft = length - i;
        const auto b0 = dataPtr[i];
        const auto b1 = bytesLeft > 1 ? dataPtr[i + 1] : 0;
        const auto b2 = bytesLeft > 2 ? dataPtr[i + 2] : 0;
        output.push_back(cb64[b0 >> 2]);
        output.push_back(cb64[((b0 & 0x03) << 4) | ((b1 & 0xf0) >> 4)]);
        output.push_back((bytesLeft > 1 ? cb64[((b1 & 0x0f) << 2) | ((b2 & 0xc0) >> 6)] : '='));
        output.push_back((bytesLeft > 2 ? cb64[b2 & 0x3f] : '='));
    }
    return output;
}

} // namespace seasocks
