#ifndef CONDITIONER_TURING_MACHINE_H
#define CONDITIONER_TURING_MACHINE_H

#if defined(__cplusplus)
extern "C" {
#endif

#define TAPE_LIMIT 30
#define EMPTY_SYMBOL ' '
#define STOP_SYMBOL '$'

enum Direct {
    Left, Right
};

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
};

void engine_init(struct Engine *engine, int symbols, int states, char init_state);

void engine_symbol_set(struct Engine *engine, int i, char symbol);

void engine_state_set(struct Engine *engine, int i, char state);

void engine_reference_set(struct Engine *engine,
                          int i_symbol, int i_state,
                          char symbol, char state, enum Direct direct);

void engine_tape_copy(struct Engine *engine, int offset, const char *tape);

void machine(struct Engine *engine);

#if defined(__cplusplus)
}
#endif

#endif //CONDITIONER_TURING_MACHINE_H
