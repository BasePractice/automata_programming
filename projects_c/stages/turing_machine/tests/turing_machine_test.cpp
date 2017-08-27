#include <gtest/gtest.h>
#include <turing_machine.h>

TEST(TuringMachine, Call) {
    int ret = machine("tests_data/turing_machine_states", "_____");
    ASSERT_EQ(ret, 0);
}