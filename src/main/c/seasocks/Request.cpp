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

#include "seasocks/Request.h"

#include <cstring>

namespace seasocks {

const char* Request::name(Verb v) {
    switch (v) {
        case Verb::Invalid:
            return "Invalid";
        case Verb::WebSocket:
            return "WebSocket";
        case Verb::Get:
            return "Get";
        case Verb::Put:
            return "Put";
        case Verb::Post:
            return "Post";
        case Verb::Delete:
            return "Delete";
        case Verb::Head:
            return "Head";
        case Verb::Options:
            return "Options";
        default:
            return "???";
    }
}

Request::Verb Request::verb(const char* verb) {
    if (std::strcmp(verb, "GET") == 0)
        return Request::Verb::Get;
    if (std::strcmp(verb, "PUT") == 0)
        return Request::Verb::Put;
    if (std::strcmp(verb, "POST") == 0)
        return Request::Verb::Post;
    if (std::strcmp(verb, "DELETE") == 0)
        return Request::Verb::Delete;
    if (std::strcmp(verb, "HEAD") == 0)
        return Request::Verb::Head;
    if (std::strcmp(verb, "OPTIONS") == 0)
        return Request::Verb::Options;
    return Request::Verb::Invalid;
}

} // namespace seasocks
