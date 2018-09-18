#include <cstring>
#include <catch2/catch.hpp>
#include "simple_regexp.h"

TEST_CASE("10+", "[04.Regular_Expression]") {
    SECTION("Совпадает") {
        REQUIRE(match(ONE_ZERO_KLEENE, "10"));
        REQUIRE(match(ONE_ZERO_KLEENE, "100"));
        REQUIRE(match(ONE_ZERO_KLEENE, "1000"));
        REQUIRE(match(ONE_ZERO_KLEENE, "10000"));
        REQUIRE(match(ONE_ZERO_KLEENE, "100000"));
        REQUIRE(match(ONE_ZERO_KLEENE, "1000000"));
        REQUIRE(match(ONE_ZERO_KLEENE, "10000000"));
        REQUIRE(match(ONE_ZERO_KLEENE, "100000000"));
        REQUIRE(match(ONE_ZERO_KLEENE, "1000000000"));
        REQUIRE(match(ONE_ZERO_KLEENE, "10000000000"));
    }
    SECTION("Не совпадает") {
        REQUIRE(!match(ONE_ZERO_KLEENE, ""));
        REQUIRE(!match(ONE_ZERO_KLEENE, "1"));
        REQUIRE(!match(ONE_ZERO_KLEENE, "11"));
        REQUIRE(!match(ONE_ZERO_KLEENE, "110"));
        REQUIRE(!match(ONE_ZERO_KLEENE, "0"));
        REQUIRE(!match(ONE_ZERO_KLEENE, "1011"));
        REQUIRE(!match(ONE_ZERO_KLEENE, "100011"));
        REQUIRE(!match(ONE_ZERO_KLEENE, "000"));
        REQUIRE(!match(ONE_ZERO_KLEENE, "010"));
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
