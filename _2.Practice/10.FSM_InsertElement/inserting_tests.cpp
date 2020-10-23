#include <cstring>
#include <catch2/catch.hpp>
#include "inserting_fsm.h"

TEST_CASE("Задержка", "[05.Lexical_Analyze]") {
    struct InsertingEngine engine = {};
    char m1[] = "24578912130";
    char m2[] = "24521232478912130";

    SECTION("Инициализация.") {
        REQUIRE_FALSE(inserting_init(nullptr, nullptr, 0, nullptr, 0));
        REQUIRE_FALSE(inserting_init(&engine, nullptr, 0, nullptr, 0));
        REQUIRE_FALSE(inserting_init(&engine, m1, 0, nullptr, 0));
        REQUIRE_FALSE(inserting_init(&engine, nullptr, 0, m2, 0));
        REQUIRE(inserting_init(&engine, m1, 0, m2, 0));
    }
    SECTION("Правильная последовательность.") {
        REQUIRE(inserting_init(&engine, m1, strlen(m1), m2, strlen(m2)));
        REQUIRE(INSERTING_NEXT == inserting_engine(&engine));
        REQUIRE(INSERTING_NEXT == inserting_engine(&engine));
        REQUIRE(INSERTING_NEXT == inserting_engine(&engine));
        REQUIRE(INSERTING_NEXT == inserting_engine(&engine));
        REQUIRE(INSERTING_NEXT == inserting_engine(&engine));
        REQUIRE(INSERTING_NEXT == inserting_engine(&engine));
        REQUIRE(INSERTING_NEXT == inserting_engine(&engine));
        REQUIRE(INSERTING_NEXT == inserting_engine(&engine));
        REQUIRE(INSERTING_NEXT == inserting_engine(&engine));
        REQUIRE(INSERTING_NEXT == inserting_engine(&engine));
        REQUIRE(INSERTING_DETECT == inserting_engine(&engine));

        REQUIRE(3 == engine.c_1);
        REQUIRE(9 == engine.c_2);

        REQUIRE(INSERTING_NEXT == inserting_engine(&engine));
        REQUIRE(INSERTING_NEXT == inserting_engine(&engine));
        REQUIRE(INSERTING_NEXT == inserting_engine(&engine));
        REQUIRE(INSERTING_NEXT == inserting_engine(&engine));
        REQUIRE(INSERTING_NEXT == inserting_engine(&engine));
        REQUIRE(INSERTING_NEXT == inserting_engine(&engine));
        REQUIRE(INSERTING_NEXT == inserting_engine(&engine));
        REQUIRE(INSERTING_OK_END == inserting_engine(&engine));
    }
}
