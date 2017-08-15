#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <conditioner_i.h>
#include <conditioner.h>
#include <emulator.h>
#include <version.h>

const char *ts_file_name = "ts_emulator.t";
int _product_errors = 0;

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
    int ok;
    int ret = temperature_sensor_read(ts_file_name, &ok);
    if (ok)
        return ret;
    _product_errors = 1;
    return 0;
}

int
product_process_engine(enum EngineMode mode, int timeout) {
    static const char *text_mode[] = {
            "Cooling", "Heating", "Waiting"
    };
    fprintf(stdout, "[Engine] Mode: %s\n", text_mode[mode]);
    x_sleep(timeout);
    return ERR_OK;
}

int
product_is_running(int state) {
    return 1;
}

int
product_errors() {
    return _product_errors;
}

int
main(int argc, char **argv) {
    int ret;
    int ok;
    struct ConditionerInter inter;
    struct StorageInter storage;

    fprintf(stdout, "Version %s\n", VERSION_STRING);
    memset(&inter, 0, sizeof(struct ConditionerInter));
    temperature_sensor_write(ts_file_name, 0, &ok);
    inter.get_current_temperature = product_get_current_temperature;
    inter.process_engine = product_process_engine;
    inter.is_running = product_is_running;
    inter.base.errors = product_errors;
    storage.property_get_int = memory_storage_property_get_int;
    storage.property_load = main_storage_property_load;
    storage.property_set_int = memory_storage_property_set_int;
    storage.property_store = main_storage_property_store;
    storage.base.errors = memory_storage_errors;
    ret = business_fsm(_PowerOn, &inter, &storage);
    return ret == 1 ? EXIT_SUCCESS : EXIT_FAILURE;
}

