// Copyright (c) 2013, Matt Godbolt
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
        Verb verb,
        HeaderMap&& headers) :
            _credentials(std::shared_ptr<Credentials>(new Credentials())),
            _remoteAddress(remoteAddress),
            _requestUri(requestUri),
            _verb(verb),
            _headers(std::move(headers)),
            _contentLength(getIntHeader("Content-Length")) {
}

bool PageRequest::consumeContent(std::vector<uint8_t>& buffer) {
    if (buffer.size() < _contentLength) return false;
    if (buffer.size() == _contentLength) {
        _content.swap(buffer);
    } else {
        _content.assign(buffer.begin(), buffer.begin() + _contentLength);
        buffer.erase(buffer.begin(), buffer.begin() + _contentLength);
    }
    return true;
}

int PageRequest::getIntHeader(const std::string& name) const {
    auto iter = _headers.find(name);
    return iter == _headers.end() ? 0 : atoi(iter->second.c_str());
}

}  // namespace seasocks
