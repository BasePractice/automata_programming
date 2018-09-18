#ifndef SIMPLE_REGEXP_H_
#define SIMPLE_REGEXP_H_

#include <stdint.h>
#include <stdbool.h>

#if defined(__cplusplus)
extern "C" {
#endif

enum Type {
    ONE_ZERO_KLEENE, XYZ
};

bool match(enum Type type, const char *text);

#if defined(__cplusplus)
}
#endif

#endif //SIMPLE_REGEXP_H_
