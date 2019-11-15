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

#include "seasocks/StringUtil.h"
#include <catch2/catch.hpp>

using namespace seasocks;

TEST_CASE("case insensitive string comparison", "[ToStringTests]") {
    CHECK(caseInsensitiveSame("abc def 123", "abc def 123") == true);
    CHECK(caseInsensitiveSame("abc def 123", "abc ghi 123") == false);
    CHECK(caseInsensitiveSame("abc", "aBc") == true);
    CHECK(caseInsensitiveSame("", "") == true);
    CHECK(caseInsensitiveSame("123", "123") == true);
    CHECK(caseInsensitiveSame(" ", " ") == true);
}

TEST_CASE("replace replaces match if found", "[ToStringTests]") {
    std::string str = "x-?-z";
    replace(str, "-?-", "y");
    CHECK(str == "xyz");
}

TEST_CASE("replace replaces all matches", "[ToStringTests]") {
    std::string str = "1xx2 xx34xxxx";
    replace(str, "xx", "---");
    CHECK(str == "1---2 ---34------");
}

TEST_CASE("replace does nothing if no match", "[ToStringTests]") {
    std::string str = "no match in here";
    replace(str, "xx", "no!");
    CHECK(str == "no match in here");
}

TEST_CASE("replace is safe to empty strings", "[ToStringTests]") {
    std::string empty{};
    replace(empty, "a", "b");
    CHECK(empty == "");

    std::string str = "input text";
    replace(str, "", "aaa");
    CHECK(str == "input text");

    replace(str, " text", "");
    CHECK(str == "input");
}

TEST_CASE("split splits input if delimiter found", "[ToStringTests]") {
    using Catch::Matchers::Equals;

    const auto result = split("123-456-789-0", '-');
    CHECK_THAT(result, Equals<std::string>({"123", "456", "789", "0"}));
}

TEST_CASE("split returns input if delimiter not found", "[ToStringTests]") {
    using Catch::Matchers::Equals;

    const auto result = split("1-2 3-4", 'x');
    CHECK_THAT(result, Equals<std::string>({"1-2 3-4"}));
}

TEST_CASE("split is safe to empty strings", "[ToStringTests]") {
    using Catch::Matchers::Equals;

    const auto result = split("", '-');
    CHECK_THAT(result, Equals<std::string>({}));
}

TEST_CASE("split returns empty strings if input is delimiter", "[ToStringTests]") {
    using Catch::Matchers::Equals;

    const auto result = split("-", '-');
    CHECK_THAT(result, Equals<std::string>({"", ""}));
}

TEST_CASE("trim whitespace removes whitespace on both ends", "[ToStringTests]") {
    const auto result = trimWhitespace(" abc def   ");
    CHECK(result == "abc def");
}

TEST_CASE("trim whitespace removes whitespace escape sequences", "[ToStringTests]") {
    const auto result = trimWhitespace("\t xyz\n");
    CHECK(result == "xyz");
}

TEST_CASE("trim whitespace is safe to empty string", "[ToStringTests]") {
    const auto result = trimWhitespace("");
    CHECK(result == "");
}

TEST_CASE("trim whitespace returns empty string if only whitespace", "[ToStringTests]") {
    const auto result = trimWhitespace(" \n \t   ");
    CHECK(result == "");
}

TEST_CASE("format address", "[ToStringTests]") {
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(12345);
    address.sin_addr.s_addr = 0x100007f;
    CHECK(formatAddress(address) == "127.0.0.1:12345");
}
