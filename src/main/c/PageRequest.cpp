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

#include "internal/PageRequest.h"

#include <cstdlib>
#include <cstring>

namespace seasocks {

PageRequest::PageRequest(
    const sockaddr_in& remoteAddress,
    const std::string& requestUri,
    Server& server,
    Verb verb,
    HeaderMap&& headers)
        : _credentials(std::make_shared<Credentials>()),
          _remoteAddress(remoteAddress),
          _requestUri(requestUri),
          _server(server),
          _verb(verb),
          _headers(std::move(headers)),
          _contentLength(getUintHeader("Content-Length")) {
}

bool PageRequest::consumeContent(std::vector<uint8_t>& buffer) {
    if (buffer.size() < _contentLength) {
        return false;
    }
    if (buffer.size() == _contentLength) {
        _content.swap(buffer);
    } else {
        _content.assign(buffer.begin(), buffer.begin() + _contentLength);
        buffer.erase(buffer.begin(), buffer.begin() + _contentLength);
    }
    return true;
}

size_t PageRequest::getUintHeader(const std::string& name) const {
    const auto iter = _headers.find(name);
    if (iter == _headers.end()) {
        return 0u;
    }
    const auto val = std::stoi(iter->second);
    if (val < 0) {
        return 0u;
    }
    return static_cast<size_t>(val);
}

} // namespace seasocks
