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

#include <string>
#include <unordered_map>
#include <vector>

namespace seasocks {

class CrackedUri {
    std::vector<std::string> _path;
    std::unordered_multimap<std::string, std::string> _queryParams;

public:
    explicit CrackedUri(const std::string& uri);

    const std::vector<std::string>& path() const {
        return _path;
    }
    const std::unordered_multimap<std::string, std::string> queryParams() const {
        return _queryParams;
    }

    bool hasParam(const std::string& param) const;

    // Returns the user-supplied default if not present. Returns the first found in the case of multiple params.
    std::string queryParam(const std::string& param, const std::string& def = std::string()) const;

    std::vector<std::string> allQueryParams(const std::string& param) const;

    // Returns a new uri with the frontmost path item shifted off.  The path
    // is always guaranteed to be non-empty. In the case of shifting the last
    // path element, returns a path of {""}.
    CrackedUri shift() const;
};

}
