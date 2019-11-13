// Copyright (c) 2019, offa
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

#include <catch2/catch.hpp>
#include "seasocks/Request.h"

using seasocks::Request;

TEST_CASE("request verb to name", "[RequestTest]") {
    using Catch::Matchers::Equals;
    using V = Request::Verb;

    CHECK_THAT(Request::name(V::Invalid), Equals("Invalid"));
    CHECK_THAT(Request::name(V::WebSocket), Equals("WebSocket"));
    CHECK_THAT(Request::name(V::Get), Equals("Get"));
    CHECK_THAT(Request::name(V::Put), Equals("Put"));
    CHECK_THAT(Request::name(V::Post), Equals("Post"));
    CHECK_THAT(Request::name(V::Delete), Equals("Delete"));
    CHECK_THAT(Request::name(V::Head), Equals("Head"));
    CHECK_THAT(Request::name(V::Options), Equals("Options"));
}

TEST_CASE("request verb from string", "[RequestTest]") {
    using V = Request::Verb;

    CHECK(Request::verb("GET") == V::Get);
    CHECK(Request::verb("PUT") == V::Put);
    CHECK(Request::verb("POST") == V::Post);
    CHECK(Request::verb("DELETE") == V::Delete);
    CHECK(Request::verb("HEAD") == V::Head);
    CHECK(Request::verb("OPTIONS") == V::Options);

    CHECK(Request::verb("") == V::Invalid);
    CHECK(Request::verb("invalid") == V::Invalid);
    CHECK(Request::verb("abc") == V::Invalid);
}
