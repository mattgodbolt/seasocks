
#include "internal/Embedded.h"
#include <catch2/catch.hpp>

TEST_CASE("findEmbeddedContent returns nullptr if not found", "[EmbeddedContentTests]") {
    const auto* content = findEmbeddedContent("/not-included");
    CHECK(content == nullptr);
}

TEST_CASE("generated content not empty", "[EmbeddedContentTests]") {
    const auto* content = findEmbeddedContent("/_404.png");
    CHECK(content != nullptr);
    CHECK(content->length > 0);
}

TEST_CASE("all files generated", "[EmbeddedContentTests]") {
    CHECK(findEmbeddedContent("/_404.png") != nullptr);
    CHECK(findEmbeddedContent("/_error.css") != nullptr);
    CHECK(findEmbeddedContent("/_error.html") != nullptr);
    CHECK(findEmbeddedContent("/favicon.ico") != nullptr);
    CHECK(findEmbeddedContent("/_jquery.min.js") != nullptr);
    CHECK(findEmbeddedContent("/_seasocks.css") != nullptr);
    CHECK(findEmbeddedContent("/_stats.html") != nullptr);
}
