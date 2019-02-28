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

#include "seasocks/util/CrackedUri.h"
#include <algorithm>

#include <catch2/catch.hpp>

using namespace seasocks;

namespace {

TEST_CASE("shouldHandleRoot", "[CrackedUriTests]") {
    CrackedUri uri("/");
    CHECK(uri.path() == std::vector<std::string>());
    CHECK(uri.queryParams().empty());
}

TEST_CASE("shouldHandleTopLevel", "[CrackedUriTests]") {
    CrackedUri uri("/monkey");
    std::vector<std::string> expected = {"monkey"};
    CHECK(uri.path() == expected);
    CHECK(uri.queryParams().empty());
}

TEST_CASE("shouldHandleSimplePaths", "[CrackedUriTests]") {
    CrackedUri uri("/foo/bar/baz/bungo");
    std::vector<std::string> expected = {"foo", "bar", "baz", "bungo"};
    CHECK(uri.path() == expected);
    CHECK(uri.queryParams().empty());
}

TEST_CASE("shouldTreatTrailingSlashAsNewPage", "[CrackedUriTests]") {
    CrackedUri uri("/ooh/a/directory/");
    std::vector<std::string> expected = {"ooh", "a", "directory", ""};
    CHECK(uri.path() == expected);
    CHECK(uri.queryParams().empty());
}

TEST_CASE("shouldHandleRootWithQuery", "[CrackedUriTests]") {
    CrackedUri uri("/?a=spotch");
    CHECK(uri.path() == std::vector<std::string>());
    REQUIRE_FALSE(uri.queryParams().empty());
    CHECK(uri.hasParam("a"));
    CHECK(uri.queryParam("a") == "spotch");
}

TEST_CASE("shouldHonorDefaults", "[CrackedUriTests]") {
    CrackedUri uri("/?a=spotch");
    CHECK(uri.queryParam("notThere", "monkey") == "monkey");
}

TEST_CASE("shouldHandleEmptyParams", "[CrackedUriTests]") {
    CrackedUri uri("/?a&b&c");
    CHECK(uri.queryParam("a", "notEmptyDefault") == "");
    CHECK(uri.queryParam("b", "notEmptyDefault") == "");
    CHECK(uri.queryParam("c", "notEmptyDefault") == "");
}

TEST_CASE("shouldHandleDuplicateParams", "[CrackedUriTests]") {
    CrackedUri uri("/?a=a&q=10&q=5&z=yibble&q=100&q=blam");
    std::vector<std::string> expected = {"10", "5", "100", "blam"};
    sort(expected.begin(), expected.end());
    auto params = uri.allQueryParams("q");
    sort(params.begin(), params.end());
    CHECK(params == expected);
}


TEST_CASE("shouldHandlePathWithQuery", "[CrackedUriTests]") {
    CrackedUri uri("/badger/badger/badger/mushroom?q=snake");
    std::vector<std::string> expected = {"badger", "badger", "badger", "mushroom"};
    CHECK(uri.path() == expected);
    REQUIRE_FALSE(uri.queryParams().empty());
    CHECK(uri.hasParam("q"));
    CHECK(uri.queryParam("q") == "snake");
}

TEST_CASE("shouldUnescapePaths", "[CrackedUriTests]") {
    CrackedUri uri("/foo+bar/baz%2f/%40%4F");
    std::vector<std::string> expected = {"foo bar", "baz/", "@O"};
    CHECK(uri.path() == expected);
}

TEST_CASE("shouldUnescapeQueries", "[CrackedUriTests]") {
    CrackedUri uri("/?q=a+b&t=%20%20&p=%41");
    CHECK(uri.queryParam("q") == "a b");
    CHECK(uri.queryParam("t") == "  ");
    CHECK(uri.queryParam("p") == "A");
}


TEST_CASE("shouldThrowOnRubbish", "[CrackedUriTests]") {
    CHECK_THROWS(CrackedUri(""));
    CHECK_THROWS(CrackedUri("rubbish"));
    CHECK_THROWS(CrackedUri("../../.."));
    CHECK_THROWS(CrackedUri("/?a===b"));
    CHECK_THROWS(CrackedUri("/%"));
    CHECK_THROWS(CrackedUri("/%a"));
    CHECK_THROWS(CrackedUri("/%qq"));
}

TEST_CASE("shouldShift", "[CrackedUriTests]") {
    CrackedUri uri("/a/b/c.html");
    std::vector<std::string> expected1 = {"a", "b", "c.html"};
    CHECK(uri.path() == expected1);

    uri = uri.shift();
    std::vector<std::string> expected2 = {"b", "c.html"};
    CHECK(uri.path() == expected2);

    uri = uri.shift();
    std::vector<std::string> expected3 = {"c.html"};
    CHECK(uri.path() == expected3);

    uri = uri.shift();
    std::vector<std::string> expected4 = {""};
    CHECK(uri.path() == expected4);

    uri = uri.shift();
    CHECK(uri.path() == expected4);
}

}
