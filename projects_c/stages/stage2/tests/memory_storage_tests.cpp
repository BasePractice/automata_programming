#include <gtest/gtest.h>
#include <emulator.h>

TEST(EmulatorStorage, Load) {
    ASSERT_TRUE(memory_storage_property_load());
    ASSERT_EQ(memory_storage_property_get_int(Temperature), 0);
    ASSERT_EQ(memory_storage_property_get_int(WaitingCooling), 0);
    ASSERT_EQ(memory_storage_property_get_int(WaitingHeating), 0);
    ASSERT_EQ(memory_storage_property_get_int(WaitingIdle), 0);
}

TEST(EmulatorStorage, LoadFromMemory) {
    int m[] = {1, 2, 3, 4};
    ASSERT_TRUE(memory_storage_property_load_from_memory((const unsigned char *const) m, sizeof(m)));
    ASSERT_EQ(memory_storage_property_get_int(Temperature), 1);
    ASSERT_EQ(memory_storage_property_get_int(WaitingCooling), 2);
    ASSERT_EQ(memory_storage_property_get_int(WaitingHeating), 3);
    ASSERT_EQ(memory_storage_property_get_int(WaitingIdle), 4);
}

TEST(EmulatorStorage, LoadFromFile) {
    ASSERT_TRUE(memory_storage_property_load_from_file(TESTS_DATA_DIRECTORY "conditioner_001.blob"));
    ASSERT_EQ(memory_storage_property_get_int(Temperature), 255);
    ASSERT_EQ(memory_storage_property_get_int(WaitingCooling), 254);
    ASSERT_EQ(memory_storage_property_get_int(WaitingHeating), 253);
    ASSERT_EQ(memory_storage_property_get_int(WaitingIdle), 252);
}

TEST(EmulatorStorage, PropertySet) {
    ASSERT_TRUE(memory_storage_property_load());
    ASSERT_EQ(memory_storage_property_get_int(Temperature), 0);
    memory_storage_property_set_int(Temperature, 1);
    ASSERT_EQ(memory_storage_property_get_int(Temperature), 1);
}

TEST(EmulatorStorage, Store) {
    ASSERT_TRUE(memory_storage_property_store());
}

TEST(EmulatorStorage, StoreToFile) {
    ASSERT_TRUE(memory_storage_property_load());
    memory_storage_property_set_int(Temperature, 1);
    memory_storage_property_set_int(WaitingCooling, 2);
    memory_storage_property_set_int(WaitingHeating, 3);
    memory_storage_property_set_int(WaitingIdle, 4);
    ASSERT_EQ(memory_storage_property_get_int(Temperature), 1);
    ASSERT_EQ(memory_storage_property_get_int(WaitingCooling), 2);
    ASSERT_EQ(memory_storage_property_get_int(WaitingHeating), 3);
    ASSERT_EQ(memory_storage_property_get_int(WaitingIdle), 4);
    ASSERT_TRUE(memory_storage_property_store_to_file(TESTS_DATA_DIRECTORY "conditioner_test.blob"));
}


