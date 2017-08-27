#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "turing_machine.h"

int
main(int argc, char *argv[]) {
    return machine(argv[1], argv[2]) == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
