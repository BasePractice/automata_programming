#ifndef CONDITIONER_EMULATOR_H
#define CONDITIONER_EMULATOR_H
#include <conditioner_i.h>

#if defined(__cplusplus)
extern "C" {
#endif

int
memory_storage_property_load();
int
memory_storage_property_load_from_memory(const unsigned char *block, int block_size);
int
memory_storage_property_load_from_file(const char *file_name);
int
memory_storage_property_store();
int
memory_storage_property_store_to_file(const char *file_name);
int
memory_storage_property_get_int(enum Property key);
void
memory_storage_property_set_int(enum Property key, int value);

#if defined(__cplusplus)
}
#endif

#endif //CONDITIONER_EMULATOR_H
