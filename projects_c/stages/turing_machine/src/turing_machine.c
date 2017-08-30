#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "turing_machine.h"

static void
char_list_new(struct CharList *list, int size) {
    assert(list != NULL);
    list->size = size;
    list->list = (char *) malloc(size * sizeof(char *));
}

static void
char_list_set(struct CharList *list, int i, char value) {
    assert(list != NULL);
    assert(list->list != NULL);
    assert(list->size > i && i >= 0);
    list->list[i] = value;
}

static int
char_list_find(struct CharList *list, char ch) {
    int i;
    for (i = 0; i < list->size; ++i) {
        if (list->list[i] == ch)
            return i;
    }
    assert(0);
}

void
engine_init(struct Engine *engine, int symbols, int states, char init_state) {
    int i = 0;
    engine->references = (struct Reference **) malloc(symbols * sizeof(struct Reference *));
    for (i = 0; i < symbols; ++i) {
        engine->references[i] = (struct Reference *) malloc(sizeof(struct Reference) * states);
    }
    engine->state = init_state;
    engine->current_i = TAPE_LIMIT / 2;
    char_list_new(&engine->states, states);
    char_list_new(&engine->symbols, symbols);
    memset(engine->tape, EMPTY_SYMBOL, sizeof(engine->tape));
}

void
engine_symbol_set(struct Engine *engine, int i, char symbol) {
    char_list_set(&engine->symbols, i, symbol);
}

void
engine_state_set(struct Engine *engine, int i, char state) {
    char_list_set(&engine->states, i, state);
}

void
engine_reference_set(struct Engine *engine,
                     int i_symbol, int i_state,
                     char symbol, char state, enum Direct direct) {
    engine->references[i_symbol][i_state].symbol = symbol;
    engine->references[i_symbol][i_state].state = state;
    engine->references[i_symbol][i_state].direct = direct;
}

void
engine_tape_copy(struct Engine *engine, int offset, const char *tape) {
    memcpy(engine->tape + offset, tape, strlen(tape));
}

void
machine(struct Engine *engine) {
    while (1) {
        int i = engine->current_i;
        char tape_symbol = engine->tape[i];
        int i_symbol = char_list_find(&engine->symbols, tape_symbol);
        int i_state = char_list_find(&engine->states, engine->state);
        struct Reference *ref = &(engine->references[i_symbol][i_state]);
        engine->tape[i] = ref->symbol;
        if (ref->state == STOP_SYMBOL)
            break;
        engine->state = ref->state;
        if (ref->direct == Left) {
            engine->current_i--;
        } else {
            engine->current_i++;
        }
    }
}
