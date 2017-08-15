#include <string.h>
#include <stdio.h>
#include <conditioner_i.h>

static int _properties[_PropertyCount] = {
        0, 0, 0, 0
};

static int _errors = 0;

int
memory_storage_property_load() {
    memset(_properties, 0, sizeof(_properties));
    return 1;
}

int
memory_storage_property_store() {
    return 1;
}

int
memory_storage_property_get_int(enum Property key) {
    return _properties[key];
}

void
memory_storage_property_set_int(enum Property key, int value) {
    _properties[key] = value;
}

int
memory_storage_property_load_from_memory(const unsigned char *const block, int block_size) {
    int i;
    if (block_size < sizeof(_properties))
        return 0;
    for (i = 0; i < sizeof(_properties) / sizeof(_properties[0]); ++i) {
        _properties[i] = ((int *) block)[i];
    }
    return 1;
}

int
memory_storage_property_load_from_file(const char *const file_name) {
    FILE *fd;

    fd = fopen(file_name, "rb");
    if (fd != 0) {
        fread(_properties, 1, sizeof(_properties), fd);
        fclose(fd);
        return 1;
    }
    return 0;
}

int
memory_storage_property_store_to_file(const char *const file_name) {
    FILE *fd;

    fd = fopen(file_name, "wb+");
    if (fd != 0) {
        fwrite(_properties, 1, sizeof(_properties), fd);
        fclose(fd);
        return 1;
    }
    return 0;
}

void
memory_storage_set_errors(int value) {
    _errors = value;
}

void
memory_storage_clear_errors() {
    memory_storage_set_errors(0);
}

int
memory_storage_errors() {
    return _errors;
}
