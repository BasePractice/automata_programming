#include <cstring>
#include <catch2/catch.hpp>
#include "delay_fsm.h"

TEST_CASE("Задержка", "[05.Lexical_Analyze]") {
    struct DelayEngine engine = {};

    SECTION("Инициализация.") {
        REQUIRE_FALSE(delay_init(nullptr));
        REQUIRE(delay_init(&engine));
        REQUIRE(engine.state == DEFAULT_DELAY_STATE);
    }
    SECTION("Сброс.") {
        REQUIRE_FALSE(delay_reset(nullptr));
        REQUIRE(delay_reset(&engine));
        REQUIRE(engine.state == DEFAULT_DELAY_STATE);
    }
    SECTION("Первый символ.") {
        REQUIRE(delay_init(&engine));
        REQUIRE(DELAY_OUTPUT_ZERO == delay_engine(&engine, DELAY_INPUT_ZERO));
        REQUIRE(delay_reset(&engine));
        REQUIRE(DELAY_OUTPUT_ZERO == delay_engine(&engine, DELAY_INPUT_ONE));
    }
    SECTION("Правильная последовательность.") {
        REQUIRE(delay_init(&engine));
        REQUIRE(DELAY_OUTPUT_ZERO == delay_engine(&engine, DELAY_INPUT_ZERO));
        REQUIRE(DELAY_OUTPUT_ZERO == delay_engine(&engine, DELAY_INPUT_ZERO));
        REQUIRE(DELAY_OUTPUT_ZERO == delay_engine(&engine, DELAY_INPUT_ONE));
        REQUIRE(DELAY_OUTPUT_ONE == delay_engine(&engine, DELAY_INPUT_ZERO));
        REQUIRE(DELAY_OUTPUT_ZERO == delay_engine(&engine, DELAY_INPUT_ONE));
        REQUIRE(DELAY_OUTPUT_ONE == delay_engine(&engine, DELAY_INPUT_ZERO));
    }
    SECTION("Ошибочная последовательнотсь.") {
        REQUIRE(delay_init(&engine));
        REQUIRE(DELAY_OUTPUT_ZERO == delay_engine(&engine, DELAY_INPUT_ZERO));
        REQUIRE(DELAY_OUTPUT_ZERO == delay_engine(&engine, DELAY_INPUT_ONE));
        REQUIRE(DELAY_OUTPUT_ONE == delay_engine(&engine, DELAY_INPUT_ZERO));
        REQUIRE(DELAY_OUTPUT_ZERO == delay_engine(&engine, DELAY_INPUT_ONE));
        REQUIRE(DELAY_OUTPUT_ONE == delay_engine(&engine, DELAY_INPUT_ZERO));
        REQUIRE_FALSE(DELAY_OUTPUT_ONE == delay_engine(&engine, DELAY_INPUT_ZERO));
    }
}
