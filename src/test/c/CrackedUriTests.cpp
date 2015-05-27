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

#include "seasocks/util/CrackedUri.h"

#include <gmock/gmock.h>

using namespace testing;
using namespace seasocks;
using namespace std;

namespace {

TEST(CrackedUriTests, shouldHandleRoot) {
    CrackedUri uri("/");
    EXPECT_EQ(vector<string>(), uri.path());
    EXPECT_TRUE(uri.queryParams().empty());
}

TEST(CrackedUriTests, shouldHandleTopLevel) {
    CrackedUri uri("/monkey");
    vector<string> expected = { "monkey" };
    EXPECT_EQ(expected, uri.path());
    EXPECT_TRUE(uri.queryParams().empty());
}

TEST(CrackedUriTests, shouldHandleSimplePaths) {
    CrackedUri uri("/foo/bar/baz/bungo");
    vector<string> expected = { "foo", "bar", "baz", "bungo" };
    EXPECT_EQ(expected, uri.path());
    EXPECT_TRUE(uri.queryParams().empty());
}

TEST(CrackedUriTests, shouldTreatTrailingSlashAsNewPage) {
    CrackedUri uri("/ooh/a/directory/");
    vector<string> expected = { "ooh", "a", "directory", "" };
    EXPECT_EQ(expected, uri.path());
    EXPECT_TRUE(uri.queryParams().empty());
}

TEST(CrackedUriTests, shouldHandleRootWithQuery) {
    CrackedUri uri("/?a=spotch");
    EXPECT_EQ(vector<string>(), uri.path());
    ASSERT_FALSE(uri.queryParams().empty());
    EXPECT_TRUE(uri.hasParam("a"));
    EXPECT_EQ("spotch", uri.queryParam("a"));
}

TEST(CrackedUriTests, shouldHonorDefaults) {
    CrackedUri uri("/?a=spotch");
    EXPECT_EQ("monkey", uri.queryParam("notThere", "monkey"));
}

TEST(CrackedUriTests, shouldHandleEmptyParams) {
    CrackedUri uri("/?a&b&c");
    EXPECT_EQ("", uri.queryParam("a", "notEmptyDefault"));
    EXPECT_EQ("", uri.queryParam("b", "notEmptyDefault"));
    EXPECT_EQ("", uri.queryParam("c", "notEmptyDefault"));
}

TEST(CrackedUriTests, shouldHandleDuplicateParams) {
    CrackedUri uri("/?a=a&q=10&q=5&z=yibble&q=100&q=blam");
    vector<string> expected = { "10", "5", "100", "blam" };
    sort(expected.begin(), expected.end());
    auto params = uri.allQueryParams("q");
    sort(params.begin(), params.end());
    EXPECT_EQ(expected, params);
}


TEST(CrackedUriTests, shouldHandlePathWithQuery) {
    CrackedUri uri("/badger/badger/badger/mushroom?q=snake");
    vector<string> expected = { "badger", "badger", "badger", "mushroom" };
    EXPECT_EQ(expected, uri.path());
    ASSERT_FALSE(uri.queryParams().empty());
    EXPECT_TRUE(uri.hasParam("q"));
    EXPECT_EQ("snake", uri.queryParam("q"));
}

TEST(CrackedUriTests, shouldUnescapePaths) {
    CrackedUri uri("/foo+bar/baz%2f/%40%4F");
    vector<string> expected = { "foo bar", "baz/", "@O" };
    EXPECT_EQ(expected, uri.path());
}

TEST(CrackedUriTests, shouldUnescapeQueries) {
    CrackedUri uri("/?q=a+b&t=%20%20&p=%41");
    EXPECT_EQ("a b", uri.queryParam("q"));
    EXPECT_EQ("  ", uri.queryParam("t"));
    EXPECT_EQ("A", uri.queryParam("p"));
}


TEST(CrackedUriTests, shouldThrowOnRubbish) {
    EXPECT_THROW(CrackedUri(""), exception);
    EXPECT_THROW(CrackedUri("rubbish"), exception);
    EXPECT_THROW(CrackedUri("../../.."), exception);
    EXPECT_THROW(CrackedUri("/?a===b"), exception);
    EXPECT_THROW(CrackedUri("/%"), exception);
    EXPECT_THROW(CrackedUri("/%a"), exception);
    EXPECT_THROW(CrackedUri("/%qq"), exception);
}

TEST(CrackedUriTests, shouldShift) {
    CrackedUri uri("/a/b/c.html");
    vector<string> expected1 = {"a", "b", "c.html"};
    EXPECT_EQ(expected1, uri.path());

    uri = uri.shift();
    vector<string> expected2 = {"b", "c.html"};
    EXPECT_EQ(expected2, uri.path());

    uri = uri.shift();
    vector<string> expected3 = {"c.html"};
    EXPECT_EQ(expected3, uri.path());

    uri = uri.shift();
    vector<string> expected4 = {""};
    EXPECT_EQ(expected4, uri.path());

    uri = uri.shift();
    EXPECT_EQ(expected4, uri.path());
}

}
