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

#include "seasocks/ToString.h"

#include <catch2/catch.hpp>

using namespace seasocks;

TEST_CASE("withIntegralTypes", "[toStringTests]") {
    CHECK(toString(static_cast<short>(1234)) == "1234");
    CHECK(toString(static_cast<unsigned short>(1234)) == "1234");

    CHECK(toString(static_cast<int>(1234)) == "1234");
    CHECK(toString(static_cast<unsigned int>(1234)) == "1234");

    CHECK(toString(static_cast<long>(1234)) == "1234");
    CHECK(toString(static_cast<unsigned long>(1234)) == "1234");

    CHECK(toString(static_cast<long long>(1234)) == "1234");
    CHECK(toString(static_cast<unsigned long long>(1234)) == "1234");
}

TEST_CASE("withFloatTypes", "[toStringTests]") {
    CHECK(toString(123.4) == "123.4");
    CHECK(toString(123.4f) == "123.4");
}

TEST_CASE("withCharType", "[toStringTests]") {
    CHECK(toString('c') == "c");
}
