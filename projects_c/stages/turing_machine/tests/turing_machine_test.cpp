#include <gtest/gtest.h>
#include <turing_machine.h>


/*
 States : A, B
 Symbols: 0, 1, E, $

           0          1           E
  A     1(B)->R    0(B)->R     E(A)->R
  B     1(B)->R    0(B)->R     E($)->R
 */

TEST(TuringMachine, Change_0_To_1_To_0) {
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

    int offset = (TAPE_LIMIT / 2) + 2;
    engine_tape_copy(engine, offset, "110010");
    machine(engine);
    engine_tape_set(engine, offset + 6, (char) 0);
    ASSERT_STREQ(engine_type_get(engine, offset), "001101");
    engine_destroy(&engine);
    ASSERT_TRUE(engine == nullptr);
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

TEST(TuringMachine, Sorting_A_B_Blocks) {
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
    engine_tape_copy(engine, offset, "ababbaaab");
    engine_offset_set(engine, offset);
    machine(engine);
    engine_tape_set(engine, offset + 9, (char) 0);
    ASSERT_STREQ(engine_type_get(engine, offset), "aaaaabbbb");
    engine_destroy(&engine);
    ASSERT_TRUE(engine == nullptr);
}