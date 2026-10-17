#ifndef STATECRAFT_MT_SESSION_H
#define STATECRAFT_MT_SESSION_H

/**
 * @file
 * Лекция 10. Пошаговый прогон машины Тьюринга для терминальной программы
 * mt_tui: машина из каталога, лента, такт вперёд и назад, восстановление
 * прошлых конфигураций для диаграммы «лента во времени».
 *
 * Как и сеансы практики 11 (life_session.h, ant_session.h), модуль не знает
 * о терминале: программа только рисует и переводит нажатия в вызовы отсюда,
 * поэтому всё, что она умеет, проверяется тестом.
 *
 * Интерпретатор здесь свой, а не `struct Engine` из turing_machine.h. Тот оставлен
 * учебным: лента на #TAPE_LIMIT ячеек, однобуквенные состояния и прогон
 * только до остановки — его ограничения и есть задания лекции. Для показа
 * нужно другое: шаг, откат и лента, на которой помещается умножение. Чтобы
 * два интерпретатора не разошлись, машины лекции записаны в каталоге той же
 * таблицей, что в mt_programs.c, и тест прогоняет обе версии на всех коротких
 * словах.
 *
 * Такт — применение одной команды: запись символа, сдвиг и смена состояния.
 * Переход в заключительное состояние тоже такт, и сдвиг в нём выполняется —
 * так считает такты Радо, и так в каталоге получаются известные значения
 * функции усердного бобра (6, 21 и 107 тактов).
 */

#if defined(__cplusplus)
extern "C" {
#endif

#define MT_TAPE_CELLS 2048 /**< ячеек ленты сеанса; слово кладётся в середину */
#define MT_MAX_STATES 24   /**< состояний в программе, вместе с заключительным */
#define MT_HISTORY 65536   /**< сколько тактов помнит сеанс для шага назад */
#define MT_INPUT_SIZE 128  /**< длина входного слова с завершающим нулём */
#define MT_VERDICT_YES '+' /**< распознаватель: слово принято */
#define MT_VERDICT_NO '-'  /**< распознаватель: слово не принято */

/** Команда машины: «состояние, символ → запись, сдвиг, состояние». */
struct MtRule {
    const char *state; /**< текущее состояние */
    char read;         /**< символ под головкой */
    char write;        /**< что записать */
    char move;         /**< 'L', 'R' или 'S' — стоять */
    const char *next;  /**< следующее состояние */
};

/** Где стоит головка в начале: на первом или на последнем символе слова. */
enum MtStart { MT_HEAD_FIRST, MT_HEAD_LAST };

/** Программа каталога. Все поля — неизменяемые данные. */
struct MtProgram {
    const char *title;           /**< название для меню */
    const char *group;           /**< раздел меню: откуда машина */
    const char *note;            /**< одна строка о том, на что смотреть */
    const char *source;          /**< откуда таблица */
    const char *legend;          /**< роли состояний: «q1 — …, q2 — …» */
    const char *alphabet;        /**< ленточный алфавит; первый символ — пустой */
    const char *input_alphabet;  /**< из чего можно набирать входное слово */
    const char *start;           /**< начальное состояние */
    const char *halt;            /**< заключительное состояние */
    int head;                    /**< enum #MtStart */
    int recognizer;              /**< 1 — вердикт пишется под головкой (#MT_VERDICT_YES) */
    const char *const *examples; /**< входные слова, NULL в конце; "" — пустое слово */
    const struct MtRule *rules;
    int rule_count;
};

/** Чем кончился прогон. */
enum MtStatus {
    MT_RUNNING,  /**< машина ещё работает */
    MT_HALTED,   /**< пришла в заключительное состояние */
    MT_ACCEPTED, /**< распознаватель остановился на #MT_VERDICT_YES */
    MT_REJECTED, /**< распознаватель остановился на #MT_VERDICT_NO */
    MT_NO_RULE,  /**< для пары «состояние, символ» команды нет */
    MT_TAPE_END  /**< головка упёрлась в край ленты сеанса */
};

/** Что нужно, чтобы отменить один такт. */
struct MtUndo {
    int pos;      /**< где стояла головка до такта */
    int state;    /**< состояние до такта */
    int rule;     /**< какая команда применена */
    int changed;  /**< 1 — символ в ячейке изменился */
    char symbol;  /**< символ, который был в ячейке */
    long written; /**< прежнее значение MtSession::written[pos] */
};

/**
 * Сеанс: программа, разобранная в таблицу, лента и история.
 *
 * Поля-признаки — int, а не bool: структуру читают тесты на C++, где у bool
 * другая раскладка (см. base_types.h).
 */
struct MtSession {
    const struct MtProgram *program;
    int state_count;
    const char *states[MT_MAX_STATES]; /**< имена; [0] — начальное */
    int halt_state;
    short table[MT_MAX_STATES][256]; /**< номер команды или -1 */

    char input[MT_INPUT_SIZE];
    char tape[MT_TAPE_CELLS];
    char initial[MT_TAPE_CELLS]; /**< лента до первого такта */
    /** Такт, на котором ячейка изменилась в последний раз; 0 — не менялась. */
    long written[MT_TAPE_CELLS];
    int origin; /**< ячейка первого символа слова: от неё считаются номера */
    int head;
    int state;
    long step;
    int status;    /**< enum #MtStatus */
    int last_rule; /**< команда последнего такта; -1 — тактов не было */

    struct MtUndo *history; /**< кольцо из #MT_HISTORY записей */
    int history_head;       /**< куда ляжет следующая запись */
    int history_count;      /**< сколько тактов можно отменить */
};

/** Пустой сеанс без программы. NULL — нет памяти; сеанс держит историю
    (около 1,5 МБ) и живёт в куче. */
struct MtSession *mt_session_create(void);

void mt_session_destroy(struct MtSession *s);

/**
 * Разобрать программу и поставить на ленту первый пример.
 *
 * Разбор и есть проверка каталога: команда с символом вне алфавита, две
 * команды на одну пару «состояние, символ», сдвиг не из «L R S», лишние
 * состояния или пример со словом вне входного алфавита — ошибка.
 *
 * @return 1 — программа загружена; 0 — она описана с ошибкой, сеанс пуст.
 */
int mt_session_load(struct MtSession *s, const struct MtProgram *program);

/**
 * Новое входное слово и прогон с такта 0.
 *
 * @return 1 — слово поставлено; 0 — в нём символ вне входного алфавита или
 *         оно длиннее #MT_INPUT_SIZE - 1. Сеанс при этом не меняется.
 */
int mt_session_set_input(struct MtSession *s, const char *word);

/** Вернуть ленту к входному слову, такт 0. */
void mt_session_reset(struct MtSession *s);

/** Такт вперёд. @return 1 — сделан; 0 — машина уже остановилась. */
int mt_session_step(struct MtSession *s);

/**
 * Не больше @p limit тактов, пока машина работает.
 *
 * @return сколько тактов сделано; меньше @p limit — машина остановилась.
 */
long mt_session_run(struct MtSession *s, long limit);

/**
 * Такт назад.
 *
 * @return 0 — отменять нечего: такт 0 либо история исчерпана
 *         (#MT_HISTORY тактов). Сеанс при этом не меняется.
 */
int mt_session_back(struct MtSession *s);

/** Команда, которая будет применена следующим тактом; -1 — её нет или машина
    остановилась. */
int mt_session_next_rule(const struct MtSession *s);

/** Номер состояния по имени; -1 — такого нет. */
int mt_session_state_index(const struct MtSession *s, const char *name);

/**
 * Конфигурация @p back тактов назад: ячейки [@p from, @p from + @p count)
 * ленты, положение головки и состояние. Сеанс не меняется — прошлое
 * собирается откатом копии.
 *
 * @param s       сеанс
 * @param back    сколько тактов назад; 0 — текущая конфигурация
 * @param from    первая ячейка окна (индекс в MtSession::tape)
 * @param count   ячеек в окне
 * @param cells   куда положить символы окна, @p count штук
 * @param head    куда записать положение головки
 * @param state   куда записать номер состояния
 * @param changed куда записать ячейку, изменённую тактом, который привёл к
 *                этой конфигурации; -1 — не изменилась ни одна
 * @return 0 — так далеко история не помнит
 */
int mt_session_view(const struct MtSession *s, int back, int from, int count, char *cells,
                    int *head, int *state, int *changed);

/**
 * Результат на ленте: от первого непустого символа до последнего, пустые
 * ячейки внутри сохраняются.
 *
 * @return @p out, либо NULL, если результат длиннее @p size - 1.
 */
const char *mt_session_result(const struct MtSession *s, char *out, int size);

/** Сколько на ленте символов @p symbol (для бобров — единиц). */
int mt_session_count(const struct MtSession *s, char symbol);

/** Пустой символ программы. */
char mt_session_blank(const struct MtSession *s);

/* --- каталог -------------------------------------------------------------- */

/** Программа каталога по номеру от нуля; NULL — номера кончились. */
const struct MtProgram *mt_catalog(int index);

int mt_catalog_count(void);

#if defined(__cplusplus)
}
#endif

#endif /* STATECRAFT_MT_SESSION_H */
