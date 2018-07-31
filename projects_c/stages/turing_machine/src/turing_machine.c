#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "turing_machine.h"

struct Reference {
    char symbol;
    char state;
    enum Direct direct;
};

struct CharList {
    char *list;
    int size;
};

struct Engine {
    struct CharList symbols;
    struct CharList states;
    struct Reference **references;
    char tape[TAPE_LIMIT];
    char state;
    int current_i;
    int state_c;
    int symbol_c;
};

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

struct Engine *
engine_create(int symbols, int states, char init_state) {
    int i;
    struct Engine *engine;

    engine = calloc(sizeof(struct Engine), 1);
    engine->references = (struct Reference **) calloc(symbols * sizeof(struct Reference *), 1);
    for (i = 0; i < symbols; ++i) {
        engine->references[i] = (struct Reference *) calloc(sizeof(struct Reference) * states, 1);
    }
    engine->state = init_state;
    engine->current_i = TAPE_LIMIT / 2;
    char_list_new(&engine->states, states);
    char_list_new(&engine->symbols, symbols);
    memset(engine->tape, EMPTY_SYMBOL, sizeof(engine->tape));
    return engine;
}

void
engine_symbol_add(struct Engine *engine, char symbol) {
    char_list_set(&engine->symbols, engine->symbol_c++, symbol);
}

void
engine_state_add(struct Engine *engine, char state) {
    char_list_set(&engine->states, engine->state_c++, state);
}

void engine_reference_add(struct Engine *engine,
                          char c_symbol, char c_state,
                          char symbol, char state, enum Direct direct) {
    int i_symbol = char_list_find(&engine->symbols, c_symbol);
    int i_state = char_list_find(&engine->states, c_state);
    engine->references[i_symbol][i_state].symbol = symbol;
    engine->references[i_symbol][i_state].state = state;
    engine->references[i_symbol][i_state].direct = direct;
}

void
engine_tape_copy(struct Engine *engine, int offset, const char *tape) {
    memcpy(engine->tape + offset, tape, strlen(tape));
}

void
engine_tape_set(struct Engine *engine, int offset, const char character) {
    engine->tape[offset] = character;
}

char *
engine_type_get(struct Engine *engine, int offset) {
    return engine->tape + offset;
}

void
engine_offset_set(struct Engine *engine, int offset) {
    assert(offset >= 0 && offset < TAPE_LIMIT);
    engine->current_i = offset;
}

void
engine_destroy(struct Engine **engine) {
    if (engine != NULL && (*engine) != NULL) {
        free((*engine));
        (*engine) = NULL;
    }
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
        if (ref->state == STOP_STATE)
            break;
        engine->state = ref->state;
        if (ref->direct == Left) {
            engine->current_i--;
        } else if (ref->direct == Right) {
            engine->current_i++;
        } else {
            /*Stay*/
        }
    }
}
