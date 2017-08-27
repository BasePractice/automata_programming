#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "turing_machine.h"

#define MAX_LENGTH 1000
#define STR_LENGTH 100
#define LIMIT 5
#define FILENAME_LENGTH 20

static char string[STR_LENGTH];
static int head, string_length;

void update_string(char, char);

int
machine(const char *file_name, const char *input_string) {
    int limit, i, flag;
    char current_state[MAX_LENGTH][LIMIT], new_state[MAX_LENGTH][LIMIT];
    char input_symbol[MAX_LENGTH], write_symbol[MAX_LENGTH], move[MAX_LENGTH];
    char state[LIMIT];
    FILE *fd;

    fd = fopen(file_name, "r");
    if (fd == NULL) {
        printf("File not found: \'%s\'\n", file_name);
        return 1;
    }
    for (limit = 0; limit < MAX_LENGTH; limit++) {
        fscanf(fd, "%s", current_state[limit]);
        if (!strcmp(current_state[limit], "$"))
            break;
        fscanf(fd, " %c %c %c %s\n", &input_symbol[limit], &write_symbol[limit], &move[limit], new_state[limit]);
    }
    strcpy(string, input_string);
    string_length = strlen(string);
    head = 0;
    strcpy(state, current_state[0]);
    while (1) {
        flag = 0;
        for (i = 0; i < limit; i++) {
            if (!strcmp(state, current_state[i]) && string[head] == input_symbol[i]) {
                update_string(write_symbol[i], move[i]);
                strcpy(state, new_state[i]);
                printf("%-10s\t|  state = %s\n", string, new_state[i]);
                if (!strcmp(state, "#"))
                    flag = 2;
                else
                    flag = 1;
                break;
            }
        }
        if (flag == 0) {
            printf("No production found for input symbol \'%c\' at state \'%s\'. Turing Machine halted!\n",
                   string[head], state);
            break;
        } else if (flag == 2) {
            printf("Accepted! Turing Machine halted!\n");
            break;
        }
    }
    return 0;
}

void
update_string(char symbol, char move) {
    int i;
    string[head] = symbol;
    if (move == 'r')
        head++;
    else if (move == 'l')
        head--;
    if (head == -1) {
        for (i = string_length; i > 0; i--)
            string[i] = string[i - 1];
        string[0] = '_';
        string_length++;
        string[string_length] = '\0';
        head = 0;
    } else if (head == string_length) {
        string[string_length] = '_';
        string_length++;
        string[string_length] = '\0';
    }
}
