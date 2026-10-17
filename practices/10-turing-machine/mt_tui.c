/*
 * Лекция 10. Машина Тьюринга в терминале: прогон такт за тактом, вперёд и
 * назад, каталог машин (лекция, классика, усердные бобры) и своё входное
 * слово.
 *
 *   → ←      такт вперёд и назад            p      пуск и пауза
 *   b e      к началу и до остановки        + -    быстрее и медленнее
 *   n        следующий пример               i      своё входное слово
 *   o        каталог машин                  q      выход
 *
 * Экран сверху вниз: лента во времени (каждая строка — конфигурация после
 * такта, новые снизу), лента крупно с головкой, таблица переходов (строка —
 * символ, столбец — состояние, как в лекции) и строка о текущем такте.
 *
 * Цвета: голубым — головка и команда, которая будет применена следующей;
 * жёлтым — ячейка, изменённая последним тактом, и последняя применённая
 * команда; оранжевым — символы, которые отличаются от входного слова;
 * серым — пустые ячейки. Вердикт распознавателя — зелёным или красным.
 *
 * Без терминала программа умеет только снимок кадра (--shot), им же
 * проверяется отрисовка (--check-shots, тест ctest).
 *
 * Здесь только терминал: прогон, откат, восстановление прошлых конфигураций
 * и каталог лежат в mt_session.c и проверяются тестом. Программа не входит в
 * обязательную сборку (STATECRAFT_BUILD_TUI): терминальный слой (tui_term.c)
 * построен на POSIX.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mt_session.h"
#include "tui_term.h"

#define IDLE_MS 250      /* как часто проверять размер окна в покое */
#define MESSAGE_MS 3000  /* сколько держится сообщение */
#define RUN_LIMIT 100000 /* «до остановки» — не дольше стольких тактов */
#define CELL 3           /* знакомест на ячейку ленты */
#define GUTTER 14        /* слева от ленты: номер такта и состояние */
#define MIN_COLS 64
#define MIN_ROWS 18
#define MIN_HISTORY 3
#define MIN_TABLE 3
#define FIXED_ROWS 11 /* всё, кроме ленты во времени и строк таблицы */

/* Цвета палитры из 256. */
#define COLOR_CELL_A 236 /* фон ячеек ленты: чередуются, чтобы видеть границы */
#define COLOR_CELL_B 238
#define COLOR_HEAD 51     /* головка, команда следующего такта */
#define COLOR_HEAD_DIM 30 /* головка в прошлых конфигурациях */
#define COLOR_WRITTEN 220 /* изменено последним тактом */
#define COLOR_WRITTEN_DIM 136
#define COLOR_CHANGED 208 /* отличается от входного слова */
#define COLOR_BLANK 242   /* пустая ячейка */
#define COLOR_DIM 245     /* подписи */
#define COLOR_YES 41      /* слово принято */
#define COLOR_NO 196      /* слово не принято */
#define COLOR_STUCK 170   /* нет команды, край ленты */
#define COLOR_INK 16      /* текст на ярком фоне */

static const int TICKS_MS[] = {20, 60, 120, 250, 500, 1000};
#define TICK_LEVELS ((int)(sizeof(TICKS_MS) / sizeof(TICKS_MS[0])))

enum Overlay { OVERLAY_NONE, OVERLAY_CATALOG, OVERLAY_INPUT };

struct App {
    struct MtSession *session;
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
    char edit[MT_INPUT_SIZE];
    char message[256];
    long message_until;
};

/* --- текст ---------------------------------------------------------------- */

static void say(struct App *app, const char *text) {
    strncpy(app->message, text, sizeof(app->message) - 1);
    app->message[sizeof(app->message) - 1] = '\0';
    app->message_until = tui_now_ms() + MESSAGE_MS;
}

/* Как символ ленты выглядит на экране: пустой «_» — точкой, как a0 в
   лекции не выглядит ни один другой символ. */
static const char *glyph(char ch) {
    static char one[2];

    if (ch == '_') {
        return "\302\267"; /* · */
    }
    one[0] = ch;
    one[1] = '\0';
    return one;
}

static void emit_glyph(struct TuiScreen *s, char ch) {
    tui_emit(s, glyph(ch));
}

/* Текст команды для таблицы: «X R q2»; @p compact — без пробелов. */
static void rule_text(const struct MtRule *r, bool compact, char *out) {
    sprintf(out, compact ? "%s%c%s" : "%s %c %s", glyph(r->write), r->move, r->next);
}

static const char *status_text(int status) {
    switch (status) {
    case MT_HALTED:
        return "остановилась";
    case MT_ACCEPTED:
        return "слово принято";
    case MT_REJECTED:
        return "слово не принято";
    case MT_NO_RULE:
        return "нет команды для пары «состояние, символ»";
    case MT_TAPE_END:
        return "головка упёрлась в край ленты";
    default:
        return "работает";
    }
}

static int status_color(int status) {
    switch (status) {
    case MT_ACCEPTED:
        return COLOR_YES;
    case MT_REJECTED:
        return COLOR_NO;
    case MT_HALTED:
        return COLOR_HEAD;
    case MT_RUNNING:
        return TUI_COLOR_DEFAULT;
    default:
        return COLOR_STUCK;
    }
}

/* --- раскладка ------------------------------------------------------------- */

struct Layout {
    int history_rows; /* строк ленты во времени */
    int table_rows;   /* строк таблицы без заголовка */
    int cells;        /* видимых ячеек ленты */
    int top_history;  /* первая строка ленты во времени (с заголовком) */
    int top_tape;     /* линейка над крупной лентой */
    int top_table;    /* заголовок таблицы */
};

/* Строки таблицы: символы, которые машина читает. Вердикты «+» и «-»
   только пишутся, и пустые строки под них лишь отнимали бы место. */
static int table_symbols(const struct App *app, char *out) {
    const struct MtSession *ms = app->session;
    const char *c;
    int n = 0;
    int state;

    for (c = ms->program->alphabet; *c != '\0'; ++c) {
        for (state = 0; state < ms->state_count; ++state) {
            if (ms->table[state][(unsigned char)*c] >= 0) {
                out[n++] = *c;
                break;
            }
        }
    }
    out[n] = '\0';
    return n;
}

static void layout(const struct App *app, struct Layout *l) {
    int rows = app->screen.rows;
    int free_rows = rows - FIXED_ROWS;
    char shown[256];
    int symbols = table_symbols(app, shown);

    l->table_rows = symbols;
    if (l->table_rows > free_rows - MIN_HISTORY) {
        l->table_rows = free_rows - MIN_HISTORY;
    }
    if (l->table_rows < MIN_TABLE && symbols >= MIN_TABLE) {
        l->table_rows = MIN_TABLE;
    }
    l->history_rows = free_rows - l->table_rows;
    if (l->history_rows < 1) {
        l->history_rows = 1;
    }
    l->cells = (app->screen.cols - GUTTER - 1) / CELL;
    if (l->cells < 1) {
        l->cells = 1;
    }
    l->top_history = 2;
    l->top_tape = l->top_history + 1 + l->history_rows;
    l->top_table = l->top_tape + 3;
}

/* Головка всегда видна, с запасом в две ячейки от края окна. */
static void follow_head(struct App *app, const struct Layout *l) {
    int head = app->session->head;
    int margin = (l->cells > 8) ? 2 : 0;

    if (head < app->view_from + margin || head >= app->view_from + l->cells - margin) {
        app->view_from = head - l->cells / 2;
    }
    if (app->view_from < 0) {
        app->view_from = 0;
    }
    if (app->view_from > MT_TAPE_CELLS - l->cells) {
        app->view_from = MT_TAPE_CELLS - l->cells;
    }
}

/* Слово — по центру окна: так видно, что вокруг него пустая лента. */
static void center_word(struct App *app) {
    struct Layout l;
    int length = (int)strlen(app->session->input);

    layout(app, &l);
    app->view_from = app->session->origin + length / 2 - l.cells / 2;
    follow_head(app, &l);
}

/* --- лента ----------------------------------------------------------------- */

static void draw_title(struct App *app) {
    struct TuiScreen *s = &app->screen;
    const struct MtProgram *p = app->session->program;
    char text[512];

    sprintf(text, " Машина Тьюринга │ %s, %d мс │ %d/%d %s │ слово «%s» │ %s",
            app->running ? "пуск" : "пауза", TICKS_MS[app->tick_level], app->program + 1,
            mt_catalog_count(), p->title, app->session->input, p->group);
    tui_move(s, 0, 0);
    tui_reverse(s);
    tui_emit_fitted(s, text, s->cols);
    tui_plain(s);

    sprintf(text, " %s. Таблица: %s", p->note, p->source);
    tui_move(s, 1, 0);
    tui_colors(s, COLOR_DIM, TUI_COLOR_DEFAULT);
    tui_emit_fitted(s, text, s->cols);
    tui_plain(s);
}

/* Строка ленты во времени: конфигурация @p back тактов назад. */
static void draw_history_row(struct App *app, const struct Layout *l, int row, int back) {
    struct TuiScreen *s = &app->screen;
    const struct MtSession *ms = app->session;
    char cells[1024];
    char text[64];
    int head;
    int state;
    int changed;
    int count = (l->cells < (int)sizeof(cells)) ? l->cells : (int)sizeof(cells);
    int i;

    tui_move(s, row, 0);
    if (ms->step - back < 0) {
        tui_emit_fitted(s, "", s->cols);
        return;
    }
    if (!mt_session_view(ms, back, app->view_from, count, cells, &head, &state, &changed)) {
        tui_colors(s, COLOR_DIM, TUI_COLOR_DEFAULT);
        tui_emit_fitted(s, "      …  история дальше не хранится", s->cols);
        tui_plain(s);
        return;
    }
    sprintf(text, " %7ld  ", ms->step - back);
    tui_colors(s, COLOR_DIM, TUI_COLOR_DEFAULT);
    tui_emit_fitted(s, text, 10);
    tui_emit_fitted(s, ms->states[state], 3);
    tui_colors(s, COLOR_DIM, TUI_COLOR_DEFAULT);
    tui_emit(s, "│");
    for (i = 0; i < count; ++i) {
        int pos = app->view_from + i;
        char ch = cells[i];
        int fg = TUI_COLOR_DEFAULT;
        int bg = TUI_COLOR_DEFAULT;

        if (ch == mt_session_blank(ms)) {
            fg = COLOR_BLANK;
        } else if (ch != ms->initial[pos]) {
            fg = COLOR_CHANGED;
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
        emit_glyph(s, ch);
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
    tui_emit_fitted(s, "    такт  сост  лента во времени: прошлые конфигурации, новые снизу",
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
    const struct MtSession *ms = app->session;
    char ruler[1024];
    char text[64];
    int width = l->cells * CELL;
    int head_col = -1;
    int i;

    /* Линейка: номера ячеек от первого символа слова, через пять. */
    if (width >= (int)sizeof(ruler)) {
        width = (int)sizeof(ruler) - 1;
    }
    memset(ruler, ' ', (size_t)width);
    ruler[width] = '\0';
    for (i = 0; i < l->cells; ++i) {
        int n = app->view_from + i - ms->origin;
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
    sprintf(text, " %7ld  ", ms->step);
    tui_emit_fitted(s, text, 10);
    tui_colors(s, COLOR_HEAD, TUI_COLOR_DEFAULT);
    tui_emit_fitted(s, ms->states[ms->state], 3);
    tui_colors(s, COLOR_DIM, TUI_COLOR_DEFAULT);
    tui_emit(s, "│");
    for (i = 0; i < l->cells; ++i) {
        int pos = app->view_from + i;
        char ch = ms->tape[pos];
        int bg = (pos % 2 == 0) ? COLOR_CELL_A : COLOR_CELL_B;
        int fg = TUI_COLOR_DEFAULT;

        if (ch == mt_session_blank(ms)) {
            fg = COLOR_BLANK;
        } else if (ms->program->recognizer && ch == MT_VERDICT_YES) {
            fg = COLOR_YES;
        } else if (ms->program->recognizer && ch == MT_VERDICT_NO) {
            fg = COLOR_NO;
        } else if (ch != ms->initial[pos]) {
            fg = COLOR_CHANGED;
        }
        if (ms->step > 0 && ms->written[pos] == ms->step) {
            fg = COLOR_INK;
            bg = COLOR_WRITTEN;
        }
        if (pos == ms->head) {
            fg = COLOR_INK;
            bg = COLOR_HEAD;
            head_col = GUTTER + i * CELL;
        }
        tui_colors(s, fg, bg);
        tui_emit(s, " ");
        emit_glyph(s, ch);
        tui_emit(s, " ");
    }
    tui_plain(s);
    tui_emit_fitted(s, "", s->cols - GUTTER - l->cells * CELL);

    /* Под головкой — стрелка и состояние. */
    tui_move(s, l->top_tape + 2, 0);
    tui_emit_fitted(s, "", s->cols);
    if (head_col >= 0) {
        tui_move(s, l->top_tape + 2, head_col + 1);
        tui_colors(s, COLOR_HEAD, TUI_COLOR_DEFAULT);
        sprintf(text, "\342\226\262 %s", ms->states[ms->state]); /* ▲ */
        tui_emit_fitted(s, text, s->cols - head_col - 1);
        tui_plain(s);
    }
}

/* --- таблица переходов ----------------------------------------------------- */

struct TableGeometry {
    int columns[MT_MAX_STATES]; /* какие состояния показаны */
    int column_count;
    int width; /* знакомест на столбец */
    bool compact;
    int first_symbol; /* первая показанная строка */
};

static int cell_width(const struct MtSession *ms, bool compact) {
    char text[64];
    int width = 4;
    int i;

    for (i = 0; i < ms->program->rule_count; ++i) {
        int w;
        rule_text(&ms->program->rules[i], compact, text);
        w = tui_text_width(text);
        if (w > width) {
            width = w;
        }
    }
    for (i = 0; i < ms->state_count; ++i) {
        int w = tui_text_width(ms->states[i]);
        if (w > width) {
            width = w;
        }
    }
    return width + 2;
}

static void table_geometry(const struct App *app, const struct Layout *l, struct TableGeometry *g) {
    const struct MtSession *ms = app->session;
    char shown[256];
    int symbols = table_symbols(app, shown);
    int room = app->screen.cols - 6;
    int working = ms->state_count - 1; /* у заключительного столбца нет */
    int fit;
    int first;
    int i;
    int row;

    g->compact = false;
    g->width = cell_width(ms, false);
    if (g->width * working > room) {
        g->compact = true;
        g->width = cell_width(ms, true);
    }
    fit = room / g->width;
    if (fit < 1) {
        fit = 1;
    }
    /* Не помещаются все состояния — окно вокруг текущего. */
    first = 0;
    if (working > fit) {
        int current = (ms->state == ms->halt_state) ? 0 : ms->state;
        int index = 0;
        for (i = 0; i < ms->state_count; ++i) {
            if (i == ms->halt_state) {
                continue;
            }
            if (i == current) {
                break;
            }
            ++index;
        }
        first = index - fit / 2;
        if (first > working - fit) {
            first = working - fit;
        }
        if (first < 0) {
            first = 0;
        }
    }
    g->column_count = 0;
    row = 0;
    for (i = 0; i < ms->state_count && g->column_count < fit; ++i) {
        if (i == ms->halt_state) {
            continue;
        }
        if (row++ >= first) {
            g->columns[g->column_count++] = i;
        }
    }
    /* Так же со строками: окно вокруг символа под головкой. */
    g->first_symbol = 0;
    if (symbols > l->table_rows) {
        const char *at = strchr(shown, ms->tape[ms->head]);
        int current = (at != NULL) ? (int)(at - shown) : 0;
        g->first_symbol = current - l->table_rows / 2;
        if (g->first_symbol > symbols - l->table_rows) {
            g->first_symbol = symbols - l->table_rows;
        }
        if (g->first_symbol < 0) {
            g->first_symbol = 0;
        }
    }
}

static void draw_table(struct App *app, const struct Layout *l) {
    struct TuiScreen *s = &app->screen;
    const struct MtSession *ms = app->session;
    char alphabet[256];
    struct TableGeometry g;
    int next = mt_session_next_rule(ms);
    char under = ms->tape[ms->head];
    char text[128];
    int used;
    int c;
    int r;

    table_symbols(app, alphabet);
    table_geometry(app, l, &g);
    used = 6 + g.column_count * g.width;

    tui_move(s, l->top_table, 0);
    tui_colors(s, COLOR_DIM, TUI_COLOR_DEFAULT);
    tui_emit_fitted(s, " сим", 6);
    for (c = 0; c < g.column_count; ++c) {
        int state = g.columns[c];
        tui_colors(s, (state == ms->state) ? COLOR_HEAD : COLOR_DIM, TUI_COLOR_DEFAULT);
        tui_emit_fitted(s, ms->states[state], g.width);
    }
    tui_colors(s, COLOR_DIM, TUI_COLOR_DEFAULT);
    tui_emit_fitted(s, "", s->cols - used);

    for (r = 0; r < l->table_rows; ++r) {
        int symbol = g.first_symbol + r;
        tui_move(s, l->top_table + 1 + r, 0);
        if (symbol >= (int)strlen(alphabet)) {
            tui_plain(s);
            tui_emit_fitted(s, "", s->cols);
            continue;
        }
        tui_colors(s, (alphabet[symbol] == under) ? COLOR_HEAD : COLOR_DIM, TUI_COLOR_DEFAULT);
        sprintf(text, "  %s", glyph(alphabet[symbol]));
        tui_emit_fitted(s, text, 6);
        for (c = 0; c < g.column_count; ++c) {
            int state = g.columns[c];
            int rule = ms->table[state][(unsigned char)alphabet[symbol]];
            if (rule < 0) {
                tui_colors(s, COLOR_BLANK, TUI_COLOR_DEFAULT);
                strcpy(text, "—");
            } else {
                rule_text(&ms->program->rules[rule], g.compact, text);
                if (rule == next) {
                    tui_colors(s, COLOR_INK, COLOR_HEAD);
                } else if (rule == ms->last_rule) {
                    tui_colors(s, COLOR_INK, COLOR_WRITTEN);
                } else {
                    tui_plain(s);
                }
            }
            /* Ячейка подсвечивается целиком, кроме зазора справа. */
            tui_emit(s, " ");
            tui_emit_fitted(s, text, g.width - 2);
            tui_plain(s);
            tui_emit(s, " ");
        }
        tui_emit_fitted(s, "", s->cols - used);
    }
}

/* --- строки внизу ---------------------------------------------------------- */

static void draw_info(struct App *app, const struct Layout *l) {
    struct TuiScreen *s = &app->screen;
    const struct MtSession *ms = app->session;
    int row = l->top_table + 1 + l->table_rows;
    int next = mt_session_next_rule(ms);
    char text[MT_TAPE_CELLS + 256];
    char result[MT_TAPE_CELLS + 1];

    tui_move(s, row, 0);
    tui_colors(s, COLOR_DIM, TUI_COLOR_DEFAULT);
    sprintf(text, " %s", ms->program->legend);
    tui_emit_fitted(s, text, s->cols);

    tui_move(s, row + 1, 0);
    if (ms->status == MT_RUNNING && next >= 0) {
        const struct MtRule *r = &ms->program->rules[next];
        char write[8];
        strcpy(write, glyph(r->write));
        sprintf(text, " такт %ld: в %s читает «%s» → пишет «%s», сдвиг %c, в %s", ms->step + 1,
                r->state, glyph(r->read), write, r->move, r->next);
        tui_plain(s);
    } else if (ms->status == MT_RUNNING) {
        sprintf(text, " такт %ld: в %s под головкой «%s» — команды нет", ms->step + 1,
                ms->states[ms->state], glyph(ms->tape[ms->head]));
        tui_colors(s, COLOR_STUCK, TUI_COLOR_DEFAULT);
    } else {
        int len = sprintf(text, " %s на такте %ld", status_text(ms->status), ms->step);
        if (ms->program->recognizer && (ms->status == MT_ACCEPTED || ms->status == MT_REJECTED)) {
            len += sprintf(text + len, " (вердикт «%c» под головкой)", ms->tape[ms->head]);
        } else if (ms->status == MT_HALTED
                   && mt_session_result(ms, result, (int)sizeof(result)) != NULL) {
            if (strcmp(ms->program->alphabet, "01") == 0) {
                len += sprintf(text + len, ": единиц на ленте %d", mt_session_count(ms, '1'));
            } else {
                len += sprintf(text + len, ": на ленте «%s»", result);
            }
        }
        tui_colors(s, status_color(ms->status), TUI_COLOR_DEFAULT);
    }
    tui_emit_fitted(s, text, s->cols);
    tui_plain(s);

    /* Легенда цветов — теми же цветами. */
    tui_move(s, row + 2, 0);
    tui_emit(s, " ");
    tui_colors(s, COLOR_INK, COLOR_HEAD);
    tui_emit(s, " a ");
    tui_plain(s);
    tui_emit(s, " головка, следующая команда  ");
    tui_colors(s, COLOR_INK, COLOR_WRITTEN);
    tui_emit(s, " a ");
    tui_plain(s);
    tui_emit(s, " изменено тактом  ");
    tui_colors(s, COLOR_CHANGED, TUI_COLOR_DEFAULT);
    tui_emit(s, "a");
    tui_plain(s);
    tui_emit(s, " не как во входе  ");
    tui_colors(s, COLOR_BLANK, TUI_COLOR_DEFAULT);
    tui_emit(s, glyph(mt_session_blank(ms)));
    tui_plain(s);
    tui_emit_fitted(s, " пусто", s->cols - 70);
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
    int count = mt_catalog_count();

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
    int count = mt_catalog_count();
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
    sprintf(text, " Машины (%d)   ↑↓ PgUp PgDn — выбор, Enter — загрузить, Esc — назад", count);
    tui_emit_fitted(s, text, width);
    tui_plain(s);
    tui_move(s, top + 1, left);
    tui_emit_fitted(s, "", width);
    for (i = 0; i < lines; ++i) {
        int index = app->menu_top + i;
        const struct MtProgram *p = mt_catalog(index);
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
    sprintf(text, " %s", mt_catalog(app->menu_index)->note);
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
    const struct MtProgram *p = app->session->program;
    char text[512];
    int top;
    int left;
    int width;

    if (!input_geometry(app, &top, &left, &width)) {
        return;
    }
    tui_move(s, top, left);
    tui_reverse(s);
    sprintf(text, " Входное слово: символы «%s», Enter — поставить, Esc — назад",
            p->input_alphabet);
    tui_emit_fitted(s, text, width);
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
    tui_emit_fitted(s, " Пустая строка — пустое слово. Backspace — стереть символ", width);
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
    draw_table(app, &l);
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
    if (!mt_session_load(app->session, mt_catalog(index))) {
        /* Каталог проверяется тестом; сюда попасть можно только с его дефектом. */
        say(app, "машина каталога описана с ошибкой");
        return;
    }
    app->program = index;
    app->example = 0;
    app->running = false;
    center_word(app);
}

static void next_example(struct App *app) {
    const struct MtProgram *p = app->session->program;
    char text[256];

    ++app->example;
    if (p->examples[app->example] == NULL) {
        app->example = 0;
    }
    mt_session_set_input(app->session, p->examples[app->example]);
    app->running = false;
    center_word(app);
    sprintf(text, "пример %d: «%s»", app->example + 1, p->examples[app->example]);
    say(app, text);
}

static void run_to_end(struct App *app) {
    long done;
    char text[256];

    app->running = false;
    done = mt_session_run(app->session, RUN_LIMIT);
    if (app->session->status == MT_RUNNING) {
        sprintf(text, "не остановилась за %d тактов — возможно, не остановится никогда", RUN_LIMIT);
        say(app, text);
    } else if (done == 0) {
        say(app, "машина уже остановилась: b — к началу");
    }
}

static void handle_menu_key(struct App *app, int key) {
    int count = mt_catalog_count();
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
    const struct MtProgram *p = app->session->program;
    size_t length = strlen(app->edit);
    char text[128];

    if (key == TUI_KEY_ESC) {
        app->overlay = OVERLAY_NONE;
    } else if (key == TUI_KEY_ENTER) {
        if (mt_session_set_input(app->session, app->edit)) {
            app->overlay = OVERLAY_NONE;
            app->running = false;
            center_word(app);
        } else {
            say(app, "слово не поставлено");
        }
    } else if (key == 127 || key == 8) {
        if (length > 0) {
            app->edit[length - 1] = '\0';
        }
    } else if (key > 32 && key < 127) {
        if (strchr(p->input_alphabet, key) == NULL) {
            sprintf(text, "символа «%c» нет во входном алфавите «%s»", key, p->input_alphabet);
            say(app, text);
        } else if (length + 1 < MT_INPUT_SIZE && length + 1 < 60) {
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
        if (app->session->status != MT_RUNNING) {
            say(app, "машина остановилась: b — к началу, ← — назад");
        } else {
            app->running = !app->running;
            app->next_tick = tui_now_ms();
        }
        break;
    case TUI_KEY_RIGHT:
        app->running = false;
        if (!mt_session_step(app->session)) {
            say(app, status_text(app->session->status));
        }
        break;
    case TUI_KEY_LEFT:
        app->running = false;
        if (!mt_session_back(app->session)) {
            say(app, (app->session->step == 0) ? "это такт 0: раньше него ничего нет"
                                               : "история хранит последние 65536 тактов");
        }
        break;
    case 'b':
    case 'B':
        app->running = false;
        mt_session_reset(app->session);
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
        strcpy(app->edit, app->session->input);
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
            if (!mt_session_step(app->session)) {
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
 * Отрисовка всех машин каталога в окнах разного размера, на такте 0, на
 * середине и после остановки, с открытым каталогом и окном ввода. Кадр не
 * выводится: проверяется, что он собран и не упёрся в размер буфера (иначе
 * хвост кадра молча пропал бы).
 */
static int check_shots(struct App *app) {
    static const int sizes[][2] = {{80, 24}, {64, 18}, {120, 40}, {200, 60}, {40, 10}};
    int failures = 0;
    int n;
    int k;
    int phase;

    for (n = 0; n < mt_catalog_count(); ++n) {
        for (k = 0; k < (int)(sizeof(sizes) / sizeof(sizes[0])); ++k) {
            app->screen.cols = sizes[k][0];
            app->screen.rows = sizes[k][1];
            load_program(app, n);
            for (phase = 0; phase < 5; ++phase) {
                app->overlay = OVERLAY_NONE;
                if (phase == 1) {
                    mt_session_run(app->session, 7);
                } else if (phase == 2) {
                    mt_session_run(app->session, 3000);
                } else if (phase == 3) {
                    app->overlay = OVERLAY_CATALOG;
                    app->menu_index = n;
                } else if (phase == 4) {
                    app->overlay = OVERLAY_INPUT;
                    strcpy(app->edit, app->session->input);
                }
                draw(app, false);
                if (app->screen.used + 64 >= app->screen.size) {
                    fprintf(stderr, "mt-tui: кадр «%s» %d×%d не поместился в буфер\n",
                            mt_catalog(n)->title, sizes[k][0], sizes[k][1]);
                    ++failures;
                }
            }
        }
    }
    printf("mt-tui: проверено кадров %d, ошибок %d\n",
           mt_catalog_count() * (int)(sizeof(sizes) / sizeof(sizes[0])) * 5, failures);
    return failures == 0 ? 0 : 1;
}

static void usage(void) {
    fprintf(stderr,
            "использование: 10-turing-machine-tui [--program N] [--input СЛОВО] [--steps K]\n"
            "                                     [--catalog | --edit] [--shot ШИРИНАxВЫСОТА]\n"
            "               10-turing-machine-tui --list | --check-shots\n");
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
        for (i = 0; i < mt_catalog_count(); ++i) {
            printf("%2d  %s — %s\n", i + 1, mt_catalog(i)->title, mt_catalog(i)->group);
        }
        return 0;
    }
    if (program < 0 || program >= mt_catalog_count()) {
        fprintf(stderr, "mt-tui: машины с номером %d нет, всего %d (--list)\n", program + 1,
                mt_catalog_count());
        return 2;
    }
    if (shot == NULL && !checks && !tui_is_terminal()) {
        fprintf(stderr, "mt-tui: нужен терминал — программа интерактивная (кадр без терминала — "
                        "--shot 100x30)\n");
        return 1;
    }

    memset(&app, 0, sizeof(app));
    app.session = mt_session_create();
    if (app.session == NULL) {
        fprintf(stderr, "mt-tui: не хватило памяти\n");
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
            mt_session_destroy(app.session);
            return 2;
        }
        if (shot == NULL) {
            tui_size(&app.screen.cols, &app.screen.rows);
        }
        load_program(&app, program);
        if (input != NULL && !mt_session_set_input(app.session, input)) {
            fprintf(stderr, "mt-tui: слово «%s» не из входного алфавита «%s»\n", input,
                    app.session->program->input_alphabet);
            mt_session_destroy(app.session);
            return 2;
        }
        center_word(&app);
        mt_session_run(app.session, steps);
        if (catalog) {
            app.overlay = OVERLAY_CATALOG;
            app.menu_index = program;
        } else if (edit) {
            app.overlay = OVERLAY_INPUT;
            strcpy(app.edit, app.session->input);
        }
        if (shot != NULL) {
            draw(&app, true);
            printf("\n");
        } else {
            tui_install_signals();
            if (!tui_enter()) {
                fprintf(stderr, "mt-tui: терминал не переводится в посимвольный режим\n");
                code = 1;
            } else {
                run(&app);
                tui_leave();
            }
        }
    }
    tui_screen_free(&app.screen);
    mt_session_destroy(app.session);
    return code;
}
