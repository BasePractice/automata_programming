#include <cstring>
#include <catch2/catch.hpp>
#include <turing_machine.h>

/*
 States : A, B
 Symbols: 0, 1, E, $

           0          1           E
  A     1(B)->R    0(B)->R     E(A)->R
  B     1(B)->R    0(B)->R     E($)->R
 */

TEST_CASE("Замена 0 на 1, 1 на 0", "[02.Turing_Machine]") {
    auto engine = engine_create(3, 2, 'A');

    engine_symbol_add(engine, '0');
    engine_symbol_add(engine, '1');
    engine_symbol_add(engine, EMPTY_SYMBOL);

    engine_state_add(engine, 'A');
    engine_state_add(engine, 'B');

    engine_reference_add(engine, '0', 'A', '1', 'B', Right);
    engine_reference_add(engine, '1', 'A', '0', 'B', Right);
    engine_reference_add(engine, EMPTY_SYMBOL, 'A', EMPTY_SYMBOL, 'A', Right);

    engine_reference_add(engine, '0', 'B', '1', 'B', Right);
    engine_reference_add(engine, '1', 'B', '0', 'B', Right);
    engine_reference_add(engine, EMPTY_SYMBOL, 'B', EMPTY_SYMBOL, STOP_STATE, Right);

#define TEXT "110010"
    int offset = (TAPE_LIMIT / 2) + 2;
    engine_tape_copy(engine, offset, TEXT);
    machine(engine);
    engine_tape_set(engine, offset + (int)strlen(TEXT), (char) 0);
    REQUIRE(strcmp(engine_type_get(engine, offset), "001101") == 0);
    engine_destroy(&engine);
    REQUIRE(engine == nullptr);
#undef TEXT
}

/*
 States : A, B, C, D, K
 Symbols: a, b, E, $
 A - ищем левый край строки
 B - идем по a
 C - идем по b
 D - ищем конец b
 K - меняем крайнюю букву на a

           a          b           E
  A     1(B)->R    0(B)->R     E(A)->R
  B     1(B)->R    0(B)->R     E($)->R
  C
  D
  K
 */

TEST_CASE("Упорядочивание символов", "[02.Turing_Machine]") {
    auto engine = engine_create(3, 5, 'A');

    engine_symbol_add(engine, 'a');
    engine_symbol_add(engine, 'b');
    engine_symbol_add(engine, EMPTY_SYMBOL);

    engine_state_add(engine, 'A');
    engine_state_add(engine, 'B');
    engine_state_add(engine, 'C');
    engine_state_add(engine, 'D');
    engine_state_add(engine, 'K');

    engine_reference_add(engine, 'a', 'A', 'a', 'A', Left);
    engine_reference_add(engine, 'b', 'A', 'b', 'A', Left);
    engine_reference_add(engine, EMPTY_SYMBOL, 'A', EMPTY_SYMBOL, 'B', Right);

    engine_reference_add(engine, 'a', 'B', 'a', 'B', Right);
    engine_reference_add(engine, 'b', 'B', 'b', 'C', Right);
    engine_reference_add(engine, EMPTY_SYMBOL, 'B', EMPTY_SYMBOL, STOP_STATE, Stay);

    engine_reference_add(engine, 'a', 'C', 'b', 'D', Left);
    engine_reference_add(engine, 'b', 'C', 'b', 'C', Right);
    engine_reference_add(engine, EMPTY_SYMBOL, 'C', EMPTY_SYMBOL, STOP_STATE, Stay);

    engine_reference_add(engine, 'a', 'D', 'a', 'K', Right);
    engine_reference_add(engine, 'b', 'D', 'b', 'D', Left);
    engine_reference_add(engine, EMPTY_SYMBOL, 'D', EMPTY_SYMBOL, 'K', Right);

    engine_reference_add(engine, 'b', 'K', 'a', 'B', Stay);

    int offset = (TAPE_LIMIT / 2) + 2;
#define TEXT "ababbaaab"
    engine_tape_copy(engine, offset, TEXT);
    engine_offset_set(engine, offset);
    machine(engine);
    engine_tape_set(engine, offset + (int)strlen(TEXT), (char) 0);
    REQUIRE(strcmp(engine_type_get(engine, offset), "aaaaabbbb") == 0);
    engine_destroy(&engine);
    REQUIRE(engine == nullptr);
#undef TEXT
}