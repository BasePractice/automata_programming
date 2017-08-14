#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <conditioner_i.h>
#include <conditioner.h>
#include <emulator.h>
#include <version.h>

int
main_storage_property_load() {
    if (memory_storage_property_load_from_file(TESTS_DATA_DIRECTORY "conditioner_main.blob")) {
        fprintf(stdout, "Load temperature : %d C\n", memory_storage_property_get_int(Temperature));
        fprintf(stdout, "Load wait cooling: %d ms\n", memory_storage_property_get_int(WaitingCooling));
        fprintf(stdout, "Load wait heating: %d ms\n", memory_storage_property_get_int(WaitingHeating));
        fprintf(stdout, "Load wait idle   : %d ms\n", memory_storage_property_get_int(WaitingIdle));
        return 1;
    }
    return 0;
}

int
main_storage_property_store() {
    return memory_storage_property_store_to_file(TESTS_DATA_DIRECTORY "conditioner_main.blob");
}

int
product_get_current_temperature() {
    return 0;
}

int
product_process_engine(enum EngineMode mode, int timeout) {
    x_sleep(timeout);
    return 0;
}

int
product_is_running(int state) {
    if (state == _Control)
        return 0;
    return 1;
}

int
main(int argc, char **argv) {
    int ret;
    struct ConditionerInter inter;
    struct StorageInter storage;

    fprintf(stdout, "Version %s\n", VERSION_STRING);
    memset(&inter, 0, sizeof(struct ConditionerInter));
    inter.get_current_temperature = product_get_current_temperature;
    inter.process_engine = product_process_engine;
    inter.is_running = product_is_running;
    storage.property_get_int = memory_storage_property_get_int;
    storage.property_load = main_storage_property_load;
    storage.property_set_int = memory_storage_property_set_int;
    storage.property_store = main_storage_property_store;
    storage.base.has_errors = memory_storage_has_errors;
    ret = business_fsm(_PowerOn, &inter, &storage);
    return ret == 1 ? EXIT_SUCCESS : EXIT_FAILURE;
}

