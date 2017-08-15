#include <gtest/gtest.h>
#include <conditioner.h>
#include <emulator.h>

static const int guard_temperature = 1;
static int current_temperature = 0;
static int wait_counter = 2;
static int errors = 0;

int
test_storage_property_load() {
    int m[] = {guard_temperature, 50, 50, 50};
    return memory_storage_property_load_from_memory((const unsigned char *const) m, sizeof(m));
}

int
test_get_current_temperature() {
    return current_temperature;
}

int
test_engine_machine(enum EngineMode mode, int timeout) {
    switch (mode) {
        case Cooling: {
            current_temperature--;
            break;
        }
        case Heating: {
            current_temperature++;
            break;
        }
        case Waiting:
            break;
    }
    x_sleep(timeout);
    return 1;
}

int
test_engine_failed_machine(enum EngineMode mode, int timeout) {
    errors = 1;
    x_sleep(timeout);
    return 1;
}

int
test_is_running(int state) {
    if (state == _Waiting) {
        if (wait_counter == 1) {
            current_temperature = guard_temperature + 1;
        } else if (wait_counter <= 0) {
            return 0;
        }
        wait_counter--;
    }
    return 1;
}

int
test_errors() {
    return errors;
}

TEST(Conditioner, BusinessFSM) {
    struct ConditionerInter inter;
    struct StorageInter storage;

    memset(&inter, 0, sizeof(struct ConditionerInter));
    memset(&storage, 0, sizeof(struct StorageInter));
    inter.get_current_temperature = test_get_current_temperature;
    inter.process_engine = test_engine_machine;
    inter.is_running = test_is_running;
    inter.base.errors = test_errors;
    storage.property_get_int = memory_storage_property_get_int;
    storage.property_load = test_storage_property_load;
    storage.property_set_int = memory_storage_property_set_int;
    storage.property_store = memory_storage_property_store;
    storage.base.errors = memory_storage_errors;
    memory_storage_clear_errors();
    ASSERT_EQ(business_fsm(_PowerOn, &inter, &storage), ERR_OK);
    ASSERT_EQ(current_temperature, guard_temperature);
}

TEST(Conditioner, BusinessFailed) {
    struct ConditionerInter inter;
    struct StorageInter storage;

    memset(&inter, 0, sizeof(struct ConditionerInter));
    memset(&storage, 0, sizeof(struct StorageInter));
    inter.get_current_temperature = test_get_current_temperature;
    inter.process_engine = test_engine_failed_machine;
    inter.is_running = test_is_running;
    inter.base.errors = test_errors;
    storage.property_get_int = memory_storage_property_get_int;
    storage.property_load = test_storage_property_load;
    storage.property_set_int = memory_storage_property_set_int;
    storage.property_store = memory_storage_property_store;
    storage.base.errors = memory_storage_errors;
    errors = 0;
    memory_storage_clear_errors();
    wait_counter = 2;
    ASSERT_EQ(business_fsm(_PowerOn, &inter, &storage), ERR_CONDITIONER);
    errors = 0;
    memory_storage_set_errors(1);
    wait_counter = 2;
    ASSERT_EQ(business_fsm(_PowerOn, &inter, &storage), ERR_STORAGE);
}