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

#include "seasocks/StringUtil.h"
#include "seasocks/util/CrackedUri.h"

#include <algorithm>
#include <sstream>
#include <stdexcept>

#define THROW(stuff)                         \
    do {                                     \
        std::ostringstream err;              \
        err << stuff;                        \
        throw std::runtime_error(err.str()); \
    } while (0);

namespace {

char fromHex(char c) {
    c = tolower(c);
    return c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10;
}

std::string unescape(std::string uri) {
    seasocks::replace(uri, "+", " ");
    size_t pos = 0;
    while (pos < uri.size()) {
        pos = uri.find('%', pos);
        if (pos == std::string::npos) {
            break;
        }
        if (pos + 2 > uri.size()) {
            THROW("Truncated uri: '" << uri << "'");
        }
        if (!isxdigit(uri[pos + 1]) || !isxdigit(uri[pos + 2])) {
            THROW("Bad digit in uri: '" << uri << "'");
        }
        auto hex = (fromHex(uri[pos + 1]) << 4) | fromHex(uri[pos + 2]);
        uri = uri.substr(0, pos) + std::string(1, hex) + uri.substr(pos + 3);
        ++pos;
    }
    return uri;
}

}

namespace seasocks {

CrackedUri::CrackedUri(const std::string& uri) {
    if (uri.empty() || uri[0] != '/') {
        THROW("Malformed URI: '" << uri << "'");
    }
    auto endOfPath = uri.find('?');
    std::string path;
    std::string remainder;
    if (endOfPath == std::string::npos) {
        path = uri.substr(1);
    } else {
        path = uri.substr(1, endOfPath - 1);
        remainder = uri.substr(endOfPath + 1);
    }

    _path = split(path, '/');
    std::transform(_path.begin(), _path.end(), _path.begin(), unescape);

    auto splitRemainder = split(remainder, '&');
    for (const auto& iter : splitRemainder) {
        if (iter.empty()) {
            continue;
        }
        auto split = seasocks::split(iter, '=');
        std::transform(split.begin(), split.end(), split.begin(), unescape);
        if (split.size() == 1) {
            _queryParams.insert(std::make_pair(split[0], std::string()));
        } else if (split.size() == 2) {
            _queryParams.insert(std::make_pair(split[0], split[1]));
        } else {
            THROW("Malformed URI, two many = in query: '" << uri << "'");
        }
    }
}

bool CrackedUri::hasParam(const std::string& param) const {
    return _queryParams.find(param) != _queryParams.end();
}

std::string CrackedUri::queryParam(const std::string& param, const std::string& def) const {
    auto found = _queryParams.find(param);
    return found == _queryParams.end() ? def : found->second;
}

std::vector<std::string> CrackedUri::allQueryParams(const std::string& param) const {
    std::vector<std::string> params;
    for (auto iter = _queryParams.find(param); iter != _queryParams.end() && iter->first == param; ++iter) {
        params.push_back(iter->second);
    }
    return params;
}

CrackedUri CrackedUri::shift() const {
    CrackedUri shifted(*this);
    if (_path.size() > 1) {
        shifted._path = std::vector<std::string>(_path.begin() + 1, _path.end());
    } else {
        shifted._path = {""};
    }

    return shifted;
}

}
