#include <string.h>
#include <stdio.h>
#include <conditioner_i.h>

static int properties[_PropertyCount] = {
        0, 0, 0, 0
};

int
memory_storage_property_load() {
    memset(properties, 0, sizeof(properties));
    return 1;
}

int
memory_storage_property_store() {
    return 1;
}

int
memory_storage_property_get_int(enum Property key) {
    return properties[key];
}

void
memory_storage_property_set_int(enum Property key, int value) {
    properties[key] = value;
}

int
memory_storage_property_load_from_memory(const unsigned char *const block, int block_size) {
    int i;
    if (block_size < sizeof(properties))
        return 0;
    for (i = 0; i < sizeof(properties) / sizeof(properties[0]); ++i) {
        properties[i] = ((int *) block)[i];
    }
    return 1;
}

int
memory_storage_property_load_from_file(const char *const file_name) {
    FILE *fd;

    fd = fopen(file_name, "rb");
    if (fd != 0) {
        fread(properties, 1, sizeof(properties), fd);
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
        fwrite(properties, 1, sizeof(properties), fd);
        fclose(fd);
        return 1;
    }
    return 0;
}
