/*
 * Лекция 10. Машина Поста в терминале: прогон такт за тактом, вперёд и
 * назад, каталог программ (лекция и классические задачи) и своё входное
 * слово. Пара к mt_tui — машине Тьюринга: те же клавиши и тот же экран.
 *
 *   → ←      такт вперёд и назад            p      пуск и пауза
 *   b e      к началу и до остановки        + -    быстрее и медленнее
 *   n        следующий пример               i      своё входное слово
 *   o        каталог программ               q      выход
 *
 * Экран сверху вниз: лента во времени (каждая строка — конфигурация после
 * такта, новые снизу), лента крупно с кареткой, текст программы (строка,
 * которая выполнится следующей, и последняя выполненная подсвечены) и строка
 * о текущем такте.
 *
 * Цвета: голубым — каретка и строка, которая выполнится следующей; жёлтым —
 * ячейка, изменённая последним тактом, и последняя выполненная строка;
 * оранжевым — ячейки, которые отличаются от входного слова (в том числе
 * стёртые метки); серым — пустые ячейки.
 *
 * Без терминала программа умеет только снимок кадра (--shot), им же
 * проверяется отрисовка (--check-shots, тест ctest).
 *
 * Здесь только терминал: разбор, прогон, откат и каталог лежат в
 * post_machine.c и pm_catalog.c и проверяются тестом. Программа не входит в
 * обязательную сборку (STATECRAFT_BUILD_TUI): терминальный слой (tui_term.c)
 * построен на POSIX.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "post_machine.h"
#include "tui_term.h"

#define IDLE_MS 250      /* как часто проверять размер окна в покое */
#define MESSAGE_MS 3000  /* сколько держится сообщение */
#define RUN_LIMIT 100000 /* «до остановки» — не дольше стольких тактов */
#define CELL 3           /* знакомест на ячейку ленты */
#define GUTTER 14        /* слева от ленты: номер такта и строки программы */
#define MIN_COLS 64
#define MIN_ROWS 18
#define MIN_HISTORY 3
#define MIN_LISTING 3
#define FIXED_ROWS 11 /* всё, кроме ленты во времени и строк программы */
#define COMMAND_W 9   /* столбец команды в тексте программы */

/* Цвета палитры из 256 — те же, что у машины Тьюринга. */
#define COLOR_CELL_A 236 /* фон ячеек ленты: чередуются, чтобы видеть границы */
#define COLOR_CELL_B 238
#define COLOR_HEAD 51     /* каретка, строка следующего такта */
#define COLOR_HEAD_DIM 30 /* каретка в прошлых конфигурациях */
#define COLOR_WRITTEN 220 /* изменено последним тактом */
#define COLOR_WRITTEN_DIM 136
#define COLOR_CHANGED 208 /* отличается от входного слова */
#define COLOR_BLANK 242   /* пустая ячейка */
#define COLOR_DIM 245     /* подписи */
#define COLOR_STUCK 170   /* невыполнимая команда, край ленты */
#define COLOR_INK 16      /* текст на ярком фоне */

static const int TICKS_MS[] = {20, 60, 120, 250, 500, 1000};
#define TICK_LEVELS ((int)(sizeof(TICKS_MS) / sizeof(TICKS_MS[0])))

enum Overlay { OVERLAY_NONE, OVERLAY_CATALOG, OVERLAY_INPUT };

struct App {
    struct PmMachine *machine;
    struct TuiScreen screen;
    int program; /* номер в каталоге */
    int example; /* номер примера программы */
    int tick_level;
    bool running;
    long next_tick;
    int view_from; /* первая видимая ячейка ленты */
    enum Overlay overlay;
    int menu_index;
    int menu_top;
    char edit[PM_INPUT_SIZE];
    char message[256];
    long message_until;
};

/* --- текст ---------------------------------------------------------------- */

static void say(struct App *app, const char *text) {
    strncpy(app->message, text, sizeof(app->message) - 1);
    app->message[sizeof(app->message) - 1] = '\0';
    app->message_until = tui_now_ms() + MESSAGE_MS;
}

/* Ячейка на экране: метка — «V», как её ставит команда, пустая — точкой. */
static const char *glyph(char cell) {
    return cell ? "V" : "\302\267"; /* · */
}

static const char *status_text(int status) {
    switch (status) {
    case PM_HALTED:
        return "остановилась";
    case PM_STUCK_MARK:
        return "невыполнимая команда: метка в помеченную ячейку";
    case PM_STUCK_EMPTY:
        return "невыполнимая команда: стирание в пустой ячейке";
    case PM_TAPE_END:
        return "каретка упёрлась в край ленты";
    default:
        return "работает";
    }
}

/* Что сделает команда: для строки о следующем такте. */
static void explain(const struct PmMachine *m, const struct PmCommand *c, char *out) {
    char cell = m->tape[m->head];

    switch (c->op) {
    case PM_MARK:
        if (cell) {
            strcpy(out, "поставить метку — но ячейка уже помечена: команда невыполнима");
        } else {
            sprintf(out, "поставить метку, затем к строке %d", c->next);
        }
        break;
    case PM_ERASE:
        if (!cell) {
            strcpy(out, "стереть метку — но ячейка пуста: команда невыполнима");
        } else {
            sprintf(out, "стереть метку, затем к строке %d", c->next);
        }
        break;
    case PM_LEFT:
        sprintf(out, "шаг влево, затем к строке %d", c->next);
        break;
    case PM_RIGHT:
        sprintf(out, "шаг вправо, затем к строке %d", c->next);
        break;
    case PM_TEST:
        sprintf(out, "под кареткой %s — к строке %d", cell ? "метка" : "пусто",
                cell ? c->next_marked : c->next);
        break;
    default:
        strcpy(out, "останов");
        break;
    }
}

/* --- раскладка ------------------------------------------------------------- */

struct Layout {
    int history_rows; /* строк ленты во времени */
    int listing_rows; /* строк текста программы без заголовка */
    int columns;      /* столбцов текста программы */
    int column_w;     /* знакомест на столбец */
    int cells;        /* видимых ячеек ленты */
    int top_history;  /* первая строка ленты во времени (с заголовком) */
    int top_tape;     /* линейка над крупной лентой */
    int top_listing;  /* заголовок текста программы */
};

/* Строка программы в том виде, как она стоит на экране. */
static void listing_text(const struct PmMachine *m, int line, char *out) {
    char command[32];
    char padded[COMMAND_W + 1];
    const struct PmCommand *c = &m->lines[line - 1];

    pm_command_text(c, command);
    sprintf(padded, "%-*.*s", COMMAND_W, COMMAND_W, command);
    sprintf(out, "%2d. %s %s", line, padded, c->comment);
}

static int listing_width(const struct PmMachine *m) {
    char text[PM_COMMENT_SIZE + 64];
    int width = 0;
    int i;

    for (i = 1; i <= m->line_count; ++i) {
        int w;
        listing_text(m, i, text);
        w = tui_text_width(text);
        if (w > width) {
            width = w;
        }
    }
    return width;
}

static void layout(const struct App *app, struct Layout *l) {
    int rows = app->screen.rows;
    int free_rows = rows - FIXED_ROWS;
    int lines = app->machine->line_count;
    int needed;

    l->column_w = listing_width(app->machine) + 3;
    if (l->column_w > app->screen.cols - 1) {
        l->column_w = app->screen.cols - 1;
    }
    if (l->column_w < 1) {
        l->column_w = 1;
    }
    l->columns = (app->screen.cols - 1) / l->column_w;
    if (l->columns < 1) {
        l->columns = 1;
    }
    needed = (lines + l->columns - 1) / l->columns;
    l->listing_rows = needed;
    if (l->listing_rows > free_rows - MIN_HISTORY) {
        l->listing_rows = free_rows - MIN_HISTORY;
    }
    if (l->listing_rows < MIN_LISTING && needed >= MIN_LISTING) {
        l->listing_rows = MIN_LISTING;
    }
    if (l->listing_rows < 1) {
        l->listing_rows = 1;
    }
    l->history_rows = free_rows - l->listing_rows;
    if (l->history_rows < 1) {
        l->history_rows = 1;
    }
    l->cells = (app->screen.cols - GUTTER - 1) / CELL;
    if (l->cells < 1) {
        l->cells = 1;
    }
    l->top_history = 2;
    l->top_tape = l->top_history + 1 + l->history_rows;
    l->top_listing = l->top_tape + 3;
}

/* Каретка всегда видна, с запасом в две ячейки от края окна. */
static void follow_head(struct App *app, const struct Layout *l) {
    int head = app->machine->head;
    int margin = (l->cells > 8) ? 2 : 0;

    if (head < app->view_from + margin || head >= app->view_from + l->cells - margin) {
        app->view_from = head - l->cells / 2;
    }
    if (app->view_from < 0) {
        app->view_from = 0;
    }
    if (app->view_from > PM_TAPE_CELLS - l->cells) {
        app->view_from = PM_TAPE_CELLS - l->cells;
    }
}

/* Слово — по центру окна: так видно, что вокруг него пустая лента. */
static void center_word(struct App *app) {
    struct Layout l;
    int length = 0;
    const char *c;

    for (c = app->machine->input; *c != '\0'; ++c) {
        length += (*c != PM_HEAD_SIGN);
    }
    layout(app, &l);
    app->view_from = app->machine->origin + length / 2 - l.cells / 2;
    follow_head(app, &l);
}

/* --- лента ----------------------------------------------------------------- */

static void draw_title(struct App *app) {
    struct TuiScreen *s = &app->screen;
    const struct PmProgram *p = app->machine->program;
    char text[512];

    sprintf(text, " Машина Поста │ %s, %d мс │ %d/%d %s │ слово «%s» │ %s",
            app->running ? "пуск" : "пауза", TICKS_MS[app->tick_level], app->program + 1,
            pm_catalog_count(), p->title, app->machine->input, p->group);
    tui_move(s, 0, 0);
    tui_reverse(s);
    tui_emit_fitted(s, text, s->cols);
    tui_plain(s);

    sprintf(text, " %s. Программа: %s", p->note, p->source);
    tui_move(s, 1, 0);
    tui_colors(s, COLOR_DIM, TUI_COLOR_DEFAULT);
    tui_emit_fitted(s, text, s->cols);
    tui_plain(s);
}

/* Номер такта и строки программы слева от ленты. */
static void draw_gutter(struct TuiScreen *s, long step, int line, int color) {
    char text[32];

    sprintf(text, " %7ld  ", step);
    tui_colors(s, COLOR_DIM, TUI_COLOR_DEFAULT);
    tui_emit_fitted(s, text, 10);
    sprintf(text, "%d", line);
    tui_colors(s, color, TUI_COLOR_DEFAULT);
    tui_emit_fitted(s, text, 3);
    tui_colors(s, COLOR_DIM, TUI_COLOR_DEFAULT);
    tui_emit(s, "│");
}

/* Строка ленты во времени: конфигурация @p back тактов назад. */
static void draw_history_row(struct App *app, const struct Layout *l, int row, int back) {
    struct TuiScreen *s = &app->screen;
    const struct PmMachine *m = app->machine;
    char cells[1024];
    int head;
    int line;
    int changed;
    int count = (l->cells < (int)sizeof(cells)) ? l->cells : (int)sizeof(cells);
    int i;

    tui_move(s, row, 0);
    if (m->step - back < 0) {
        tui_emit_fitted(s, "", s->cols);
        return;
    }
    if (!pm_machine_view(m, back, app->view_from, count, cells, &head, &line, &changed)) {
        tui_colors(s, COLOR_DIM, TUI_COLOR_DEFAULT);
        tui_emit_fitted(s, "      …  история дальше не хранится", s->cols);
        tui_plain(s);
        return;
    }
    draw_gutter(s, m->step - back, line, TUI_COLOR_DEFAULT);
    for (i = 0; i < count; ++i) {
        int pos = app->view_from + i;
        char cell = cells[i];
        int fg = TUI_COLOR_DEFAULT;
        int bg = TUI_COLOR_DEFAULT;

        if (cell != m->initial[pos]) {
            fg = COLOR_CHANGED;
        } else if (!cell) {
            fg = COLOR_BLANK;
        }
        if (pos == changed) {
            fg = COLOR_INK;
            bg = COLOR_WRITTEN_DIM;
        }
        if (pos == head) {
            fg = 231;
            bg = COLOR_HEAD_DIM;
        }
        tui_colors(s, fg, bg);
        tui_emit(s, " ");
        tui_emit(s, glyph(cell));
        tui_emit(s, " ");
    }
    tui_plain(s);
    tui_emit_fitted(s, "", s->cols - GUTTER - count * CELL);
}

static void draw_history(struct App *app, const struct Layout *l) {
    struct TuiScreen *s = &app->screen;
    int i;

    tui_move(s, l->top_history, 0);
    tui_colors(s, COLOR_DIM, TUI_COLOR_DEFAULT);
    tui_emit_fitted(s, "    такт  стр  лента во времени: прошлые конфигурации, новые снизу",
                    s->cols);
    tui_plain(s);
    for (i = 0; i < l->history_rows; ++i) {
        /* Самая новая — над крупной лентой, а текущая — она сама, поэтому
           ряд начинается с такта назад. */
        draw_history_row(app, l, l->top_history + 1 + i, l->history_rows - i);
    }
}

static void draw_tape(struct App *app, const struct Layout *l) {
    struct TuiScreen *s = &app->screen;
    const struct PmMachine *m = app->machine;
    char ruler[1024];
    char text[64];
    int width = l->cells * CELL;
    int head_col = -1;
    int shown_line = (m->status == PM_HALTED) ? m->last_line : m->line;
    int i;

    /* Линейка: номера ячеек от первого символа слова, через пять. */
    if (width >= (int)sizeof(ruler)) {
        width = (int)sizeof(ruler) - 1;
    }
    memset(ruler, ' ', (size_t)width);
    ruler[width] = '\0';
    for (i = 0; i < l->cells; ++i) {
        int n = app->view_from + i - m->origin;
        int len;
        if (n % 5 != 0) {
            continue;
        }
        len = sprintf(text, "%d", n);
        if (i * CELL + 1 + len <= width) {
            memcpy(ruler + i * CELL + 1, text, (size_t)len);
        }
    }
    tui_move(s, l->top_tape, 0);
    tui_colors(s, COLOR_DIM, TUI_COLOR_DEFAULT);
    tui_emit_fitted(s, "  ячейка", GUTTER);
    tui_emit_fitted(s, ruler, s->cols - GUTTER);

    /* Сама лента. */
    tui_move(s, l->top_tape + 1, 0);
    tui_plain(s);
    draw_gutter(s, m->step, shown_line, COLOR_HEAD);
    for (i = 0; i < l->cells; ++i) {
        int pos = app->view_from + i;
        char cell = m->tape[pos];
        int bg = (pos % 2 == 0) ? COLOR_CELL_A : COLOR_CELL_B;
        int fg = TUI_COLOR_DEFAULT;

        if (cell != m->initial[pos]) {
            fg = COLOR_CHANGED;
        } else if (!cell) {
            fg = COLOR_BLANK;
        }
        if (m->step > 0 && m->written[pos] == m->step) {
            fg = COLOR_INK;
            bg = COLOR_WRITTEN;
        }
        if (pos == m->head) {
            fg = COLOR_INK;
            bg = COLOR_HEAD;
            head_col = GUTTER + i * CELL;
        }
        tui_colors(s, fg, bg);
        tui_emit(s, " ");
        tui_emit(s, glyph(cell));
        tui_emit(s, " ");
    }
    tui_plain(s);
    tui_emit_fitted(s, "", s->cols - GUTTER - l->cells * CELL);

    /* Под кареткой — стрелка и номер строки программы. */
    tui_move(s, l->top_tape + 2, 0);
    tui_emit_fitted(s, "", s->cols);
    if (head_col >= 0) {
        tui_move(s, l->top_tape + 2, head_col + 1);
        tui_colors(s, COLOR_HEAD, TUI_COLOR_DEFAULT);
        sprintf(text, "\342\226\262 строка %d", shown_line); /* ▲ */
        tui_emit_fitted(s, text, s->cols - head_col - 1);
        tui_plain(s);
    }
}

/* --- текст программы ------------------------------------------------------- */

/* Первая показанная строка: всё помещается — с первой, иначе окно вокруг
   строки, которая выполнится следующей. */
static int listing_first(const struct App *app, const struct Layout *l) {
    const struct PmMachine *m = app->machine;
    int capacity = l->columns * l->listing_rows;
    int current = (m->status == PM_RUNNING) ? m->line : m->last_line;
    int first;

    if (m->line_count <= capacity) {
        return 1;
    }
    if (current < 1) {
        current = 1;
    }
    first = current - capacity / 2;
    if (first > m->line_count - capacity + 1) {
        first = m->line_count - capacity + 1;
    }
    if (first < 1) {
        first = 1;
    }
    return first;
}

static void draw_listing(struct App *app, const struct Layout *l) {
    struct TuiScreen *s = &app->screen;
    const struct PmMachine *m = app->machine;
    int next = pm_machine_next_line(m);
    int first = listing_first(app, l);
    int per_column = l->listing_rows;
    char text[PM_COMMENT_SIZE + 64];
    int r;
    int c;

    /* Всё помещается — столбцы одинаковой высоты, а не полный и огрызок. */
    if (m->line_count <= l->columns * l->listing_rows) {
        per_column = (m->line_count + l->columns - 1) / l->columns;
    }

    tui_move(s, l->top_listing, 0);
    tui_colors(s, COLOR_DIM, TUI_COLOR_DEFAULT);
    sprintf(text, " программа, строк %d: V метка, X стереть, <- -> шаг, ? ветвление, ! останов",
            m->line_count);
    tui_emit_fitted(s, text, s->cols);
    tui_plain(s);

    for (r = 0; r < l->listing_rows; ++r) {
        int used = 1;
        tui_move(s, l->top_listing + 1 + r, 0);
        tui_emit(s, " ");
        for (c = 0; c < l->columns && r < per_column; ++c) {
            int line = first + c * per_column + r;
            int prefix;
            char command[32];
            if (line > m->line_count || line >= first + l->columns * l->listing_rows) {
                break;
            }
            listing_text(m, line, text);
            pm_command_text(&m->lines[line - 1], command);
            /* Номер и команда подсвечиваются, комментарий — нет: так строка
               не превращается в сплошную цветную полосу. */
            prefix = 4 + COMMAND_W;
            if (line == next) {
                tui_colors(s, COLOR_INK, COLOR_HEAD);
            } else if (line == m->last_line) {
                tui_colors(s, COLOR_INK, COLOR_WRITTEN);
            } else {
                tui_plain(s);
            }
            tui_emit_fitted(s, text, prefix);
            tui_colors(s, (line == next) ? COLOR_HEAD : COLOR_DIM, TUI_COLOR_DEFAULT);
            tui_emit_fitted(s, text + prefix, l->column_w - prefix);
            used += l->column_w;
        }
        tui_plain(s);
        tui_emit_fitted(s, "", s->cols - used);
    }
}

/* --- строки внизу ---------------------------------------------------------- */

static void draw_info(struct App *app, const struct Layout *l) {
    struct TuiScreen *s = &app->screen;
    const struct PmMachine *m = app->machine;
    int row = l->top_listing + 1 + l->listing_rows;
    char text[PM_TAPE_CELLS + 256];
    char tape[PM_TAPE_CELLS + 2];
    char what[160];
    char command[32];

    tui_move(s, row, 0);
    tui_colors(s, COLOR_DIM, TUI_COLOR_DEFAULT);
    tui_emit_fitted(s, " Лента: · пусто, V метка; во входном слове 0, 1 и ^ перед кареткой",
                    s->cols);

    tui_move(s, row + 1, 0);
    if (m->status == PM_RUNNING) {
        const struct PmCommand *c = &m->lines[m->line - 1];
        bool stuck = (c->op == PM_MARK && m->tape[m->head])
                     || (c->op == PM_ERASE && !m->tape[m->head]);
        pm_command_text(c, command);
        explain(m, c, what);
        sprintf(text, " такт %ld: строка %d «%s» — %s", m->step + 1, m->line, command, what);
        if (stuck) {
            tui_colors(s, COLOR_STUCK, TUI_COLOR_DEFAULT);
        } else {
            tui_plain(s);
        }
    } else {
        int len;
        if (m->status == PM_HALTED) {
            len = sprintf(text, " остановилась на такте %ld, строка %d", m->step, m->last_line);
        } else {
            len = sprintf(text, " %s: такт %ld, строка %d", status_text(m->status), m->step,
                          m->line);
        }
        if (pm_machine_format(m, tape, (int)sizeof(tape)) != NULL) {
            len += sprintf(text + len, "; меток %d, лента «%s»", pm_machine_count(m), tape);
        }
        tui_colors(s, (m->status == PM_HALTED) ? COLOR_HEAD : COLOR_STUCK, TUI_COLOR_DEFAULT);
    }
    tui_emit_fitted(s, text, s->cols);
    tui_plain(s);

    /* Легенда цветов — теми же цветами. */
    tui_move(s, row + 2, 0);
    tui_emit(s, " ");
    tui_colors(s, COLOR_INK, COLOR_HEAD);
    tui_emit(s, " V ");
    tui_plain(s);
    tui_emit(s, " каретка, следующая строка  ");
    tui_colors(s, COLOR_INK, COLOR_WRITTEN);
    tui_emit(s, " V ");
    tui_plain(s);
    tui_emit(s, " изменено тактом  ");
    tui_colors(s, COLOR_CHANGED, TUI_COLOR_DEFAULT);
    tui_emit(s, "V");
    tui_plain(s);
    tui_emit(s, " не как во входе  ");
    tui_colors(s, COLOR_BLANK, TUI_COLOR_DEFAULT);
    tui_emit(s, glyph(0));
    tui_plain(s);
    tui_emit_fitted(s, " пусто", s->cols - 69);
}

static void draw_keys(struct App *app) {
    struct TuiScreen *s = &app->screen;
    char text[512];

    if (app->message[0] != '\0' && tui_now_ms() < app->message_until) {
        sprintf(text, " %s", app->message);
    } else {
        strcpy(text, " → такт  ← назад  p пуск  e до конца  b сначала  +- скорость  n пример"
                     "  i слово  o каталог  q выход");
    }
    tui_move(s, s->rows - 1, 0);
    tui_reverse(s);
    tui_emit_fitted(s, text, s->cols);
    tui_plain(s);
}

/* --- каталог и ввод слова --------------------------------------------------- */

static bool menu_geometry(const struct App *app, int *top, int *left, int *width, int *lines) {
    const struct TuiScreen *s = &app->screen;
    int count = pm_catalog_count();

    *width = (s->cols - 4 < 84) ? s->cols - 4 : 84;
    *lines = s->rows - 6;
    if (*width < 40 || *lines < 3) {
        return false;
    }
    if (*lines > count) {
        *lines = count;
    }
    *left = (s->cols - *width) / 2;
    *top = (s->rows - 1 - (*lines + 4)) / 2;
    return true;
}

static void draw_menu(struct App *app) {
    struct TuiScreen *s = &app->screen;
    int count = pm_catalog_count();
    int title_w = 32;
    int width;
    int lines;
    int left;
    int top;
    int i;
    char text[512];

    if (!menu_geometry(app, &top, &left, &width, &lines)) {
        return;
    }
    if (app->menu_index < app->menu_top) {
        app->menu_top = app->menu_index;
    }
    if (app->menu_index >= app->menu_top + lines) {
        app->menu_top = app->menu_index - lines + 1;
    }
    /* Окно выросло, пока меню было открыто: начало списка, выбранное под
       прежнюю высоту, уводило бы последние строки за конец каталога. */
    if (app->menu_top > count - lines) {
        app->menu_top = count - lines;
    }
    tui_move(s, top, left);
    tui_reverse(s);
    sprintf(text, " Программы (%d)   ↑↓ PgUp PgDn — выбор, Enter — загрузить, Esc — назад", count);
    tui_emit_fitted(s, text, width);
    tui_plain(s);
    tui_move(s, top + 1, left);
    tui_emit_fitted(s, "", width);
    for (i = 0; i < lines; ++i) {
        int index = app->menu_top + i;
        const struct PmProgram *p = pm_catalog(index);
        tui_move(s, top + 2 + i, left);
        if (index == app->menu_index) {
            tui_reverse(s);
        }
        sprintf(text, " %2d  ", index + 1);
        tui_emit_fitted(s, text, 5);
        tui_emit_fitted(s, p->title, title_w);
        tui_emit_fitted(s, p->group, width - 5 - title_w);
        tui_plain(s);
    }
    tui_move(s, top + 2 + lines, left);
    tui_emit_fitted(s, "", width);
    tui_move(s, top + 3 + lines, left);
    tui_reverse(s);
    sprintf(text, " %s", pm_catalog(app->menu_index)->note);
    tui_emit_fitted(s, text, width);
    tui_plain(s);
}

static bool input_geometry(const struct App *app, int *top, int *left, int *width) {
    *width = (app->screen.cols - 4 < 72) ? app->screen.cols - 4 : 72;
    *left = (app->screen.cols - *width) / 2;
    *top = (app->screen.rows - 1 - 4) / 2;
    return *width >= 40;
}

static void draw_input(struct App *app) {
    struct TuiScreen *s = &app->screen;
    char text[512];
    int top;
    int left;
    int width;

    if (!input_geometry(app, &top, &left, &width)) {
        return;
    }
    tui_move(s, top, left);
    tui_reverse(s);
    tui_emit_fitted(s, " Входное слово: 0 пусто, 1 метка, ^ перед кареткой; Enter, Esc", width);
    tui_plain(s);
    tui_move(s, top + 1, left);
    tui_emit_fitted(s, "", width);
    tui_move(s, top + 2, left);
    sprintf(text, "  > %s", app->edit);
    tui_emit(s, text);
    tui_colors(s, COLOR_INK, COLOR_HEAD);
    tui_emit(s, " ");
    tui_plain(s);
    tui_emit_fitted(s, "", width - tui_text_width(text) - 1);
    tui_move(s, top + 3, left);
    tui_reverse(s);
    tui_emit_fitted(s, " Без ^ каретка — на первой ячейке. Backspace — стереть символ", width);
    tui_plain(s);
}

static void draw(struct App *app, bool output) {
    struct TuiScreen *s = &app->screen;
    struct Layout l;
    int top;
    int left;
    int width;
    int lines;
    int row;

    if (!tui_frame_begin(s)) {
        return;
    }
    if (s->cols < MIN_COLS || s->rows < MIN_ROWS) {
        char text[128];
        for (row = 0; row < s->rows; ++row) {
            tui_move(s, row, 0);
            tui_emit_fitted(s, "", s->cols);
        }
        sprintf(text, "окно мало: нужно не меньше %d×%d", MIN_COLS, MIN_ROWS);
        tui_move(s, s->rows / 2, 0);
        tui_emit_fitted(s, text, s->cols);
        if (output) {
            tui_frame_end(s);
        }
        return;
    }
    layout(app, &l);
    follow_head(app, &l);
    draw_title(app);
    draw_history(app, &l);
    draw_tape(app, &l);
    draw_listing(app, &l);
    draw_info(app, &l);
    draw_keys(app);
    /* Окна поверх — последними: экран дорисован целиком, и под окном ничего
       не мелькнёт, потому что кадр выводится одной записью. */
    if (app->overlay == OVERLAY_CATALOG && menu_geometry(app, &top, &left, &width, &lines)) {
        draw_menu(app);
    } else if (app->overlay == OVERLAY_INPUT && input_geometry(app, &top, &left, &width)) {
        draw_input(app);
    }
    if (output) {
        tui_frame_end(s);
    }
}

/* --- поведение -------------------------------------------------------------- */

static void load_program(struct App *app, int index) {
    if (!pm_machine_load(app->machine, pm_catalog(index))) {
        /* Каталог проверяется тестом; сюда попасть можно только с его дефектом. */
        say(app, app->machine->error);
        return;
    }
    app->program = index;
    app->example = 0;
    app->running = false;
    center_word(app);
}

static void next_example(struct App *app) {
    const struct PmProgram *p = app->machine->program;
    char text[256];

    ++app->example;
    if (p->examples[app->example] == NULL) {
        app->example = 0;
    }
    pm_machine_set_input(app->machine, p->examples[app->example]);
    app->running = false;
    center_word(app);
    sprintf(text, "пример %d: «%s»", app->example + 1, p->examples[app->example]);
    say(app, text);
}

static void run_to_end(struct App *app) {
    long done;
    char text[256];

    app->running = false;
    done = pm_machine_run(app->machine, RUN_LIMIT);
    if (app->machine->status == PM_RUNNING) {
        sprintf(text, "не остановилась за %d тактов — возможно, не остановится никогда", RUN_LIMIT);
        say(app, text);
    } else if (app->machine->status == PM_TAPE_END) {
        say(app, "на бесконечной ленте каретка ушла бы дальше: машина не остановится");
    } else if (done == 0) {
        say(app, "машина уже остановилась: b — к началу");
    }
}

static void handle_menu_key(struct App *app, int key) {
    int count = pm_catalog_count();
    int page = app->screen.rows - 6;

    if (page < 1) {
        page = 1;
    }
    switch (key) {
    case TUI_KEY_UP:
        app->menu_index = (app->menu_index + count - 1) % count;
        break;
    case TUI_KEY_DOWN:
        app->menu_index = (app->menu_index + 1) % count;
        break;
    case TUI_KEY_PAGE_UP:
        app->menu_index = (app->menu_index > page) ? app->menu_index - page : 0;
        break;
    case TUI_KEY_PAGE_DOWN:
        app->menu_index = (app->menu_index + page < count) ? app->menu_index + page : count - 1;
        break;
    case TUI_KEY_ENTER:
        app->overlay = OVERLAY_NONE;
        load_program(app, app->menu_index);
        break;
    case TUI_KEY_ESC:
    case 'o':
    case 'O':
        app->overlay = OVERLAY_NONE;
        break;
    default:
        break;
    }
}

static void handle_input_key(struct App *app, int key) {
    size_t length = strlen(app->edit);
    char text[128];

    if (key == TUI_KEY_ESC) {
        app->overlay = OVERLAY_NONE;
    } else if (key == TUI_KEY_ENTER) {
        if (pm_machine_set_input(app->machine, app->edit)) {
            app->overlay = OVERLAY_NONE;
            app->running = false;
            center_word(app);
        } else {
            say(app, app->machine->error);
        }
    } else if (key == 127 || key == 8) {
        if (length > 0) {
            app->edit[length - 1] = '\0';
        }
    } else if (key > 32 && key < 127) {
        if (key != '0' && key != '1' && key != PM_HEAD_SIGN) {
            sprintf(text, "символ «%c»: слово набирается из 0, 1 и ^", key);
            say(app, text);
        } else if (key == PM_HEAD_SIGN && strchr(app->edit, PM_HEAD_SIGN) != NULL) {
            say(app, "каретка уже отмечена: знак ^ ставится один раз");
        } else if (length + 1 < PM_INPUT_SIZE && length + 1 < 60) {
            app->edit[length] = (char)key;
            app->edit[length + 1] = '\0';
        }
    }
}

static void handle_key(struct App *app, int key) {
    if (key == 'q' || key == 'Q') {
        if (app->overlay != OVERLAY_INPUT) {
            tui_request_stop();
            return;
        }
    }
    if (app->overlay == OVERLAY_CATALOG) {
        handle_menu_key(app, key);
        return;
    }
    if (app->overlay == OVERLAY_INPUT) {
        handle_input_key(app, key);
        return;
    }
    switch (key) {
    case 'p':
    case 'P':
    case ' ':
        if (app->machine->status != PM_RUNNING) {
            say(app, "машина остановилась: b — к началу, ← — назад");
        } else {
            app->running = !app->running;
            app->next_tick = tui_now_ms();
        }
        break;
    case TUI_KEY_RIGHT:
        app->running = false;
        if (!pm_machine_step(app->machine)) {
            say(app, status_text(app->machine->status));
        }
        break;
    case TUI_KEY_LEFT:
        app->running = false;
        if (!pm_machine_back(app->machine)) {
            say(app, (app->machine->step == 0) ? "это такт 0: раньше него ничего нет"
                                               : "история хранит последние 65536 тактов");
        }
        break;
    case 'b':
    case 'B':
        app->running = false;
        pm_machine_reset(app->machine);
        center_word(app);
        break;
    case 'e':
    case 'E':
        run_to_end(app);
        break;
    case '+':
    case '=':
        if (app->tick_level > 0) {
            --app->tick_level;
        }
        break;
    case '-':
    case '_':
        if (app->tick_level < TICK_LEVELS - 1) {
            ++app->tick_level;
        }
        break;
    case 'n':
    case 'N':
        next_example(app);
        break;
    case 'i':
    case 'I':
        strcpy(app->edit, app->machine->input);
        app->overlay = OVERLAY_INPUT;
        break;
    case 'o':
    case 'O':
        app->menu_index = app->program;
        app->overlay = OVERLAY_CATALOG;
        break;
    default:
        break;
    }
}

static void run(struct App *app) {
    while (!tui_stop_requested()) {
        int cols;
        int rows;
        int keys[16];
        int count;
        int i;
        long timeout;

        tui_size(&cols, &rows);
        if (cols != app->screen.cols || rows != app->screen.rows) {
            app->screen.cols = cols;
            app->screen.rows = rows;
            tui_clear();
        }
        draw(app, true);

        timeout = (app->running && app->overlay == OVERLAY_NONE) ? app->next_tick - tui_now_ms()
                                                                 : IDLE_MS;
        count = tui_read_keys(timeout, keys, 16);
        for (i = 0; i < count; ++i) {
            handle_key(app, keys[i]);
        }
        if (count > 0) {
            continue;
        }
        if (app->running && app->overlay == OVERLAY_NONE && tui_now_ms() >= app->next_tick) {
            if (!pm_machine_step(app->machine)) {
                app->running = false;
            }
            app->next_tick = tui_now_ms() + TICKS_MS[app->tick_level];
        }
    }
}

/* --- снимки без терминала ------------------------------------------------- */

static bool parse_size(const char *text, int *cols, int *rows) {
    return sscanf(text, "%dx%d", cols, rows) == 2 && *cols > 0 && *rows > 0 && *cols <= 400
           && *rows <= 200;
}

/*
 * Отрисовка всех программ каталога на всех примерах в окнах разного размера:
 * на такте 0, на середине и после остановки, с открытым каталогом и окном
 * ввода. Кадр не выводится: проверяется, что он собран и не упёрся в размер
 * буфера (иначе хвост кадра молча пропал бы).
 */
static int check_shots(struct App *app) {
    static const int sizes[][2] = {{80, 24}, {64, 18}, {120, 40}, {200, 60}, {40, 10}};
    int failures = 0;
    int frames = 0;
    int n;
    int k;
    int e;
    int phase;

    for (n = 0; n < pm_catalog_count(); ++n) {
        for (k = 0; k < (int)(sizeof(sizes) / sizeof(sizes[0])); ++k) {
            app->screen.cols = sizes[k][0];
            app->screen.rows = sizes[k][1];
            load_program(app, n);
            for (e = 0; pm_catalog(n)->examples[e] != NULL; ++e) {
                pm_machine_set_input(app->machine, pm_catalog(n)->examples[e]);
                center_word(app);
                for (phase = 0; phase < 5; ++phase) {
                    app->overlay = OVERLAY_NONE;
                    if (phase == 1) {
                        pm_machine_run(app->machine, 7);
                    } else if (phase == 2) {
                        pm_machine_run(app->machine, 3000);
                    } else if (phase == 3) {
                        app->overlay = OVERLAY_CATALOG;
                        app->menu_index = n;
                    } else if (phase == 4) {
                        app->overlay = OVERLAY_INPUT;
                        strcpy(app->edit, app->machine->input);
                    }
                    draw(app, false);
                    ++frames;
                    if (app->screen.used + 64 >= app->screen.size) {
                        fprintf(stderr, "pm-tui: кадр «%s» %d×%d не поместился в буфер\n",
                                pm_catalog(n)->title, sizes[k][0], sizes[k][1]);
                        ++failures;
                    }
                }
            }
        }
    }
    printf("pm-tui: проверено кадров %d, ошибок %d\n", frames, failures);
    return failures == 0 ? 0 : 1;
}

static void usage(void) {
    fprintf(stderr,
            "использование: 10-post-machine-tui [--program N] [--input СЛОВО] [--steps K]\n"
            "                                   [--catalog | --edit] [--shot ШИРИНАxВЫСОТА]\n"
            "               10-post-machine-tui --list | --check-shots\n");
}

int main(int argc, char **argv) {
    struct App app;
    const char *input = NULL;
    const char *shot = NULL;
    long steps = 0;
    bool catalog = false;
    bool edit = false;
    bool list = false;
    bool checks = false;
    int program = 0;
    int code = 0;
    int i;

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--program") == 0 && i + 1 < argc) {
            program = atoi(argv[++i]) - 1;
        } else if (strcmp(argv[i], "--input") == 0 && i + 1 < argc) {
            input = argv[++i];
        } else if (strcmp(argv[i], "--steps") == 0 && i + 1 < argc) {
            steps = atol(argv[++i]);
        } else if (strcmp(argv[i], "--shot") == 0 && i + 1 < argc) {
            shot = argv[++i];
        } else if (strcmp(argv[i], "--catalog") == 0) {
            catalog = true;
        } else if (strcmp(argv[i], "--edit") == 0) {
            edit = true;
        } else if (strcmp(argv[i], "--list") == 0) {
            list = true;
        } else if (strcmp(argv[i], "--check-shots") == 0) {
            checks = true;
        } else {
            usage();
            return 2;
        }
    }
    if (list) {
        for (i = 0; i < pm_catalog_count(); ++i) {
            printf("%2d  %s — %s\n", i + 1, pm_catalog(i)->title, pm_catalog(i)->group);
        }
        return 0;
    }
    if (program < 0 || program >= pm_catalog_count()) {
        fprintf(stderr, "pm-tui: программы с номером %d нет, всего %d (--list)\n", program + 1,
                pm_catalog_count());
        return 2;
    }
    if (shot == NULL && !checks && !tui_is_terminal()) {
        fprintf(stderr, "pm-tui: нужен терминал — программа интерактивная (кадр без терминала — "
                        "--shot 100x30)\n");
        return 1;
    }

    memset(&app, 0, sizeof(app));
    app.machine = pm_machine_create();
    if (app.machine == NULL) {
        fprintf(stderr, "pm-tui: не хватило памяти\n");
        return 1;
    }
    app.tick_level = 3;
    app.screen.cols = 100;
    app.screen.rows = 30;

    if (checks) {
        code = check_shots(&app);
    } else {
        if (shot != NULL && !parse_size(shot, &app.screen.cols, &app.screen.rows)) {
            usage();
            pm_machine_destroy(app.machine);
            return 2;
        }
        if (shot == NULL) {
            tui_size(&app.screen.cols, &app.screen.rows);
        }
        load_program(&app, program);
        if (input != NULL && !pm_machine_set_input(app.machine, input)) {
            fprintf(stderr, "pm-tui: %s\n", app.machine->error);
            pm_machine_destroy(app.machine);
            return 2;
        }
        center_word(&app);
        pm_machine_run(app.machine, steps);
        if (catalog) {
            app.overlay = OVERLAY_CATALOG;
            app.menu_index = program;
        } else if (edit) {
            app.overlay = OVERLAY_INPUT;
            strcpy(app.edit, app.machine->input);
        }
        if (shot != NULL) {
            draw(&app, true);
            printf("\n");
        } else {
            tui_install_signals();
            if (!tui_enter()) {
                fprintf(stderr, "pm-tui: терминал не переводится в посимвольный режим\n");
                code = 1;
            } else {
                run(&app);
                tui_leave();
            }
        }
    }
    tui_screen_free(&app.screen);
    pm_machine_destroy(app.machine);
    return code;
}
