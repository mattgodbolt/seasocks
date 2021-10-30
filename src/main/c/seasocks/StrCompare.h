#pragma once
// StringCompare.h, written by sjk7 (Steve) 22/10/2021
// Inherits seasocks licence.

#include <string_view>
#include <cctype>
#include <algorithm>

static inline bool compareCaseInsensitive(const std::string_view lhs,
                                          const std::string_view rhs) noexcept {
    // replaces "return strcasecmp(lhs.c_str(), rhs.c_str()) == 0" directly
    // test coverage provided by:
    // TEST_CASE("case insensitive string comparison", "[ToStringTests]")
    return ((lhs.size() == rhs.size()) &&
            std::equal(lhs.begin(), lhs.end(), rhs.begin(), [](const auto a, const auto b) noexcept {
                return std::tolower(static_cast<int>(a)) == std::tolower(static_cast<int>(b));
            }));
}
