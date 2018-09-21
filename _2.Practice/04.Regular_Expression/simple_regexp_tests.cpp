#include <cstring>
#include <catch2/catch.hpp>
#include "simple_regexp.h"

TEST_CASE("10+", "[04.Regular_Expression]") {
    SECTION("Совпадает") {
        REQUIRE(match(ONE_ZERO_PLUS, "10"));
        REQUIRE(match(ONE_ZERO_PLUS, "100"));
        REQUIRE(match(ONE_ZERO_PLUS, "1000"));
        REQUIRE(match(ONE_ZERO_PLUS, "10000"));
    }
    SECTION("Не совпадает") {
        REQUIRE_FALSE(match(ONE_ZERO_PLUS, ""));
        REQUIRE_FALSE(match(ONE_ZERO_PLUS, "1"));
        REQUIRE_FALSE(match(ONE_ZERO_PLUS, "11"));
        REQUIRE_FALSE(match(ONE_ZERO_PLUS, "110"));
        REQUIRE_FALSE(match(ONE_ZERO_PLUS, "0"));
        REQUIRE_FALSE(match(ONE_ZERO_PLUS, "1011"));
        REQUIRE_FALSE(match(ONE_ZERO_PLUS, "100011"));
        REQUIRE_FALSE(match(ONE_ZERO_PLUS, "000"));
        REQUIRE_FALSE(match(ONE_ZERO_PLUS, "010"));
    }
}

TEST_CASE("xy*|z", "[04.Regular_Expression]") {
    SECTION("Совпадает") {
        REQUIRE(match(XYZ, "xy"));
        REQUIRE(match(XYZ, "xyy"));
        REQUIRE(match(XYZ, "z"));
        REQUIRE(match(XYZ, "x"));
    }
    SECTION("Не совпадает") {
        REQUIRE(!match(XYZ, ""));
        REQUIRE(!match(XYZ, "xx"));
        REQUIRE(!match(XYZ, "xyyyx"));
        REQUIRE(!match(XYZ, "zz"));
        REQUIRE(!match(XYZ, "xzz"));
        REQUIRE(!match(XYZ, "xyyzz"));
    }
}
