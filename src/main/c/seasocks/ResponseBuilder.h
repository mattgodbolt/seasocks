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

#include "seasocks/SynchronousResponse.h"
#include "seasocks/ToString.h"

#include <sstream>
#include <string>

namespace seasocks {

class ResponseBuilder {
    ResponseCode _code;
    std::string _contentType;
    bool _keepAlive;
    SynchronousResponse::Headers _headers;
    std::shared_ptr<std::ostringstream> _stream;

public:
    explicit ResponseBuilder(ResponseCode code = ResponseCode::Ok);

    ResponseBuilder& asHtml();
    ResponseBuilder& asText();
    ResponseBuilder& asJson();
    ResponseBuilder& withContentType(const std::string& contentType);

    ResponseBuilder& keepsConnectionAlive();
    ResponseBuilder& closesConnection();

    ResponseBuilder& withLocation(const std::string& location);

    ResponseBuilder& setsCookie(const std::string& cookie, const std::string& value);

    ResponseBuilder& withHeader(const std::string& name, const std::string& value);
    template <typename T>
    ResponseBuilder& withHeader(const std::string& name, const T& t) {
        return withHeader(name, toString(t));
    }

    ResponseBuilder& addHeader(const std::string& name, const std::string& value);
    template <typename T>
    ResponseBuilder& addHeader(const std::string& name, const T& t) {
        return addHeader(name, toString(t));
    }

    template <typename T>
    ResponseBuilder& operator<<(T&& t) {
        (*_stream) << std::forward<T>(t);
        return *this;
    }

    std::shared_ptr<Response> build();
};

}
