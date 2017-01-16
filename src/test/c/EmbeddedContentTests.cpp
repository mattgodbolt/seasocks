
#include "internal/Embedded.h"
#include "catch.hpp"


TEST_CASE("generated content not empty", "[EmbeddedContentTests]") {
    const auto* content = findEmbeddedContent("/_404.png");
    CHECK(content != nullptr);
    CHECK(content->length > 0);
}

