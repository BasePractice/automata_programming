#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "delay_fsm.h"

int main(int argc, char **argv) {
    char  *sequence;
    size_t sequence_len;
    size_t sequence_it;
    struct DelayEngine engine = {DEFAULT_DELAY_STATE};

    if (argc < 2) {
        fprintf(stderr, "Передайте в качестве аргумента последовательность 0 и 1 для проверки работы элемента \"Задержка\"\n");
        return EXIT_FAILURE;
    }
    sequence =  argv[1];
    sequence_len = strlen(sequence);
    fprintf(stdout, "%s\n", sequence);
    delay_init(&engine);
    for (sequence_it = 0; sequence_it < sequence_len; ++sequence_it) {
        char it = sequence[sequence_it];
        enum DelayInputSymbol  symbol;
        enum DelayOutputSymbol output;
        if (it != '1' && it != '0') {
            fprintf(stderr, "\nСимвол %c недопустим\n", it);
            return EXIT_FAILURE;
        }
        symbol = (enum  DelayInputSymbol)it - '0';
        output = delay_engine(&engine, symbol);
        fprintf(stdout, "%c", ((int)output) + '0');
    }
    fprintf(stdout, "\n");
    return EXIT_SUCCESS;
}
