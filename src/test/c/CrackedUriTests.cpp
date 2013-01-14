#include "seasocks/util/CrackedUri.h"

#include <gmock/gmock.h>

using namespace testing;
using namespace seasocks;

namespace {

TEST(CrackedUriTests, shouldHandleRoot) {
    CrackedUri uri("/");
    EXPECT_EQ(std::vector<std::string>({""}), uri.path());
    EXPECT_TRUE(uri.queryParams().empty());
}

TEST(CrackedUriTests, shouldHandleTopLevel) {
    CrackedUri uri("/monkey");
    std::vector<std::string> expected = { "monkey" };
    EXPECT_EQ(expected, uri.path());
    EXPECT_TRUE(uri.queryParams().empty());
}

TEST(CrackedUriTests, shouldHandleSimplePaths) {
    CrackedUri uri("/foo/bar/baz/bungo");
    std::vector<std::string> expected = { "foo", "bar", "baz", "bungo" };
    EXPECT_EQ(expected, uri.path());
    EXPECT_TRUE(uri.queryParams().empty());
}

TEST(CrackedUriTests, shouldTreatTrailingSlashAsNewPage) {
    CrackedUri uri("/ooh/a/directory/");
    std::vector<std::string> expected = { "ooh", "a", "directory", "" };
    EXPECT_EQ(expected, uri.path());
    EXPECT_TRUE(uri.queryParams().empty());
}

TEST(CrackedUriTests, shouldHandleRootWithQuery) {
    CrackedUri uri("/?a=spotch");
    EXPECT_EQ(std::vector<std::string>({""}), uri.path());
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
    std::vector<std::string> expected = { "10", "5", "100", "blam" };
    std::sort(expected.begin(), expected.end());
    auto params = uri.allQueryParams("q");
    std::sort(params.begin(), params.end());
    EXPECT_EQ(expected, params);
}


TEST(CrackedUriTests, shouldHandlePathWithQuery) {
    CrackedUri uri("/badger/badger/badger/mushroom?q=snake");
    std::vector<std::string> expected = { "badger", "badger", "badger", "mushroom" };
    EXPECT_EQ(expected, uri.path());
    ASSERT_FALSE(uri.queryParams().empty());
    EXPECT_TRUE(uri.hasParam("q"));
    EXPECT_EQ("snake", uri.queryParam("q"));
}

TEST(CrackedUriTests, shouldUnescapePaths) {
    CrackedUri uri("/foo+bar/baz%2f/%40%4F");
    std::vector<std::string> expected = { "foo bar", "baz/", "@O" };
    EXPECT_EQ(expected, uri.path());
}

TEST(CrackedUriTests, shouldUnescapeQueries) {
    CrackedUri uri("/?q=a+b&t=%20%20&p=%41");
    EXPECT_EQ("a b", uri.queryParam("q"));
    EXPECT_EQ("  ", uri.queryParam("t"));
    EXPECT_EQ("A", uri.queryParam("p"));
}


TEST(CrackedUriTests, shouldThrowOnRubbish) {
    EXPECT_THROW(CrackedUri(""), std::exception);
    EXPECT_THROW(CrackedUri("rubbish"), std::exception);
    EXPECT_THROW(CrackedUri("../../.."), std::exception);
    EXPECT_THROW(CrackedUri("/?a===b"), std::exception);
    EXPECT_THROW(CrackedUri("/%"), std::exception);
    EXPECT_THROW(CrackedUri("/%a"), std::exception);
    EXPECT_THROW(CrackedUri("/%qq"), std::exception);
}

}
