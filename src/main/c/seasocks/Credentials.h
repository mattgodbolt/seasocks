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

#include <iostream>
#include <map>
#include <set>
#include <string>

namespace seasocks {

struct Credentials {
    /**
     * Whether user was successfuly authenticated.
     */
    bool authenticated;

    /**
     * e.g. "mgodbolt" (or "" for unauthenticated users)
     */
    std::string username;

    /**
     * Groups the user is in.
     */
    std::set<std::string> groups;

    /**
     * Attributes for the user.
     */
    std::map<std::string, std::string> attributes;

    Credentials()
            : authenticated(false), username(), groups(), attributes() {
    }
};

inline std::ostream& operator<<(std::ostream& os, const Credentials& credentials) {
    os << "{authenticated:" << credentials.authenticated << ", username:'" << credentials.username << "', groups: {";
    for (auto it = credentials.groups.begin(); it != credentials.groups.end(); ++it) {
        if (it != credentials.groups.begin())
            os << ", ";
        os << *it;
    }
    os << "}, attrs: {";
    for (auto it = credentials.attributes.begin(); it != credentials.attributes.end(); ++it) {
        if (it != credentials.attributes.begin())
            os << ", ";
        os << it->first << "=" << it->second;
    }
    return os << "}}";
}

} // namespace seasocks
