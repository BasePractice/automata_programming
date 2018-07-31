#ifndef CONDITIONER_TURING_MACHINE_H
#define CONDITIONER_TURING_MACHINE_H

#if defined(__cplusplus)
extern "C" {
#endif

#define TAPE_LIMIT 30
#define EMPTY_SYMBOL 'E'
#define STOP_STATE '$'

/*
 1. Имена состояний в строку
 2. Вывод в стандартный поток переходы
 3. Загрузка ленты и таблицы переходов из файла
 4. Снять ограничения с размера ленты
 5. Снять ограничения с порядка загрузки состояний и словаря (убрать предварительную инициализацию)
 6. Добавить порты ввода вывода(опционально)
 7. Добавить свои улучшения
 */

enum Direct {
    Left, Right, Stay
};

struct Engine;

struct Engine *engine_create(int symbols, int states, char init_state);

void engine_symbol_add(struct Engine *engine, char symbol);

void engine_state_add(struct Engine *engine, char state);

void engine_reference_add(struct Engine *engine,
                          char c_symbol, char c_state,
                          char symbol, char state, enum Direct direct);

void engine_tape_copy(struct Engine *engine, int offset, const char *tape);

void engine_tape_set(struct Engine *engine, int offset, char character);

void engine_offset_set(struct Engine *engine, int offset);

char *engine_type_get(struct Engine *engine, int offset);

void engine_destroy(struct Engine **engine);

void machine(struct Engine *engine);

#if defined(__cplusplus)
}
#endif

#endif //CONDITIONER_TURING_MACHINE_H
