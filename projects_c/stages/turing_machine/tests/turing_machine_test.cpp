#include <gtest/gtest.h>
#include <turing_machine.h>

TEST(TuringMachine, Call) {
    struct Engine engine{};

    memset(&engine, 0, sizeof(engine));

    engine_init(&engine, 3, 2, 'A');

    engine_symbol_set(&engine, 0, '0');
    engine_symbol_set(&engine, 1, '1');
    engine_symbol_set(&engine, 2, EMPTY_SYMBOL);

    engine_state_set(&engine, 0, 'A');
    engine_state_set(&engine, 1, 'B');

    engine_reference_set(&engine, 0, 0, '1', 'B', Right);
    engine_reference_set(&engine, 1, 0, '0', 'B', Right);
    engine_reference_set(&engine, 2, 0, EMPTY_SYMBOL, 'A', Right);

    engine_reference_set(&engine, 0, 1, '1', 'B', Right);
    engine_reference_set(&engine, 1, 1, '0', 'B', Right);
    engine_reference_set(&engine, 2, 1, EMPTY_SYMBOL, STOP_SYMBOL, Right);

    int offset = (TAPE_LIMIT / 2) + 2;
    engine_tape_copy(&engine, offset, "110010");
    machine(&engine);
    engine.tape[offset + 6] = 0;
    ASSERT_STREQ(engine.tape + offset, "001101");
}