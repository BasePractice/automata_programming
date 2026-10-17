/*
 * Лекция 11. Умный муравей в терминале: прогон автомата по тропе Санта-Фе
 * такт за тактом, каталог готовых автоматов и поиск автомата прямо на
 * экране — с показом каждого улучшения и подтверждением перехода к нему.
 *
 *   p        пауза и пуск                   ← →    такт назад и вперёд
 *   b e      к началу и к концу прогона     + -    быстрее и медленнее
 *   ↑ ↓      клетка крупнее и мельче        r      случайный автомат
 *   o        каталог автоматов              s      поиск: выбор режима
 *   Enter n  принять и отклонить найденное  a      принимать без вопросов
 *   ] [      поиск быстрее и медленнее      q      выход
 *
 * Цвета поля: серым — тропа, красным — еда, жёлтым — пройденный путь,
 * голубым — муравей. Внизу синим — таблица автомата в мнемонике
 * (ant_session.h), жёлтым — переходы, изменившиеся при последнем шаге
 * эволюции.
 *
 * Здесь только терминал: прогон, каталог, режимы поиска и сравнение
 * автоматов лежат в ant_session.c и проверяются тестом. Программа не входит
 * в обязательную сборку (STATECRAFT_BUILD_TUI): терминальный слой
 * (tui_term.c) построен на POSIX.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "ant_session.h"
#include "tui_term.h"

#define IDLE_MS 250 /* как часто проверять размер окна в покое */
#define BUSY_MS 15  /* пауза между кадрами, пока идёт поиск */
#define MESSAGE_MS 3000
#define CELL_WIDTH 16 /* знакомест на состояние в таблице автомата */
#define MIN_FIELD_ROWS 8

/* Цвета палитры из 256. */
#define COLOR_GROUND 236  /* поле вне тропы: видно, где кончается тор */
#define COLOR_TRAIL 244   /* тропа, по которой муравей ещё не шёл */
#define COLOR_FOOD 196    /* еда */
#define COLOR_VISITED 220 /* пройденный путь */
#define COLOR_ANT 51      /* муравей */
#define COLOR_TABLE 33    /* таблица автомата */
#define COLOR_CHANGED 220 /* изменения таблицы */

static const int TICKS_MS[] = {5, 10, 20, 40, 80, 160, 320};
#define TICK_LEVELS ((int)(sizeof(TICKS_MS) / sizeof(TICKS_MS[0])))
static const long BUDGETS[] = {100L, 1000L, 10000L, 50000L};
#define BUDGET_LEVELS ((int)(sizeof(BUDGETS) / sizeof(BUDGETS[0])))

enum Overlay { OVERLAY_NONE, OVERLAY_CATALOG, OVERLAY_SEARCH };

struct App {
    struct AntSession *session;
    struct TuiScreen screen;
    struct AntRandom random;
    int grain; /* желаемая сторона клетки в пикселях; на экране — не больше влезающей */
    int tick_level;
    bool running;
    enum Overlay overlay;
    int menu_index;
    int menu_top;
    char message[256];
    long message_until;
    long next_tick;

    /* поиск */
    struct AntSearchState *search;
    const struct AntSearchMode *mode;
    unsigned long seed;
    int budget_level;
    bool auto_accept;
    bool finished; /* поиск окончен, итог ещё на экране */
    bool pending;  /* найден автомат лучше, ждёт решения */
    struct AntFsm candidate;
    struct TrailRun candidate_run;
    long candidate_found; /* сколько улучшений накопилось с прошлого показа */
    long accepted;        /* сколько поколений принято */
    int candidate_limit;  /* лимит тактов постановки поиска */
};

/* --- текст ---------------------------------------------------------------- */

static const char *plural(int n, const char *one, const char *few, const char *many) {
    int n10 = n % 10;
    int n100 = n % 100;

    if (n10 == 1 && n100 != 11) {
        return one;
    }
    if (n10 >= 2 && n10 <= 4 && (n100 < 12 || n100 > 14)) {
        return few;
    }
    return many;
}

static void say(struct App *app, const char *text) {
    strncpy(app->message, text, sizeof(app->message) - 1);
    app->message[sizeof(app->message) - 1] = '\0';
    app->message_until = tui_now_ms() + MESSAGE_MS;
}

static void describe_run(const struct TrailRun *run, char *out) {
    sprintf(out, "%d/%d за %d %s", run->eaten, run->total, run->steps,
            plural(run->steps, "такт", "такта", "тактов"));
}

/* --- раскладка ------------------------------------------------------------- */

struct Layout {
    int status_row;
    int search_row;  /* -1 — строки поиска нет */
    int panel_row;   /* заголовок таблицы автомата */
    int panel_lines; /* строк самой таблицы */
    int columns;     /* состояний в строке таблицы */
    int field_rows;  /* знакомест по высоте под поле */
    int grain;       /* сторона клетки на экране */
};

static const struct AntFsm *panel_fsm(const struct App *app) {
    return app->pending ? &app->candidate : &app->session->fsm;
}

static void layout(const struct App *app, struct Layout *l) {
    int rows = app->screen.rows;
    int states = panel_fsm(app)->state_count;
    int needed;
    int fit;

    l->status_row = rows - 1;
    l->search_row = (app->search != NULL || app->finished) ? rows - 2 : -1;
    l->columns = app->screen.cols / CELL_WIDTH;
    if (l->columns < 1) {
        l->columns = 1;
    }
    needed = (states + l->columns - 1) / l->columns;
    l->panel_lines = needed;
    /* Таблица не отнимает у поля последние строки: остаток — словами. */
    l->field_rows = rows - 1 - (l->search_row >= 0 ? 1 : 0) - 1 - l->panel_lines;
    if (l->field_rows < MIN_FIELD_ROWS) {
        l->panel_lines -= MIN_FIELD_ROWS - l->field_rows;
        if (l->panel_lines < 0) {
            l->panel_lines = 0;
        }
        l->field_rows = rows - 1 - (l->search_row >= 0 ? 1 : 0) - 1 - l->panel_lines;
    }
    l->panel_row = l->field_rows;

    fit = app->screen.cols / TRAIL_SIDE;
    if (l->field_rows * 2 / TRAIL_SIDE < fit) {
        fit = l->field_rows * 2 / TRAIL_SIDE;
    }
    if (fit < 1) {
        fit = 1;
    }
    l->grain = (app->grain < fit) ? app->grain : fit;
}

static int max_grain(const struct App *app) {
    struct Layout l;
    struct App probe = *app;

    probe.grain = 1000;
    layout(&probe, &l);
    return l.grain;
}

/* --- поле ------------------------------------------------------------------ */

static int cell_color(const struct AntWalk *w, int x, int y) {
    if (x == w->x && y == w->y) {
        return COLOR_ANT;
    }
    if (w->food[y][x]) {
        return COLOR_FOOD;
    }
    if (w->visited[y][x]) {
        return COLOR_VISITED;
    }
    if (ant_trail_on_path(x, y)) {
        return COLOR_TRAIL;
    }
    return COLOR_GROUND;
}

static int pixel_color(const struct AntWalk *w, int px, int py, int ox, int oy, int grain) {
    int fx = px - ox;
    int fy = py - oy;

    if (fx < 0 || fy < 0 || fx >= TRAIL_SIDE * grain || fy >= TRAIL_SIDE * grain) {
        return TUI_PIXEL_EMPTY;
    }
    return cell_color(w, fx / grain, fy / grain);
}

static void draw_field(struct App *app, const struct Layout *l) {
    struct TuiScreen *s = &app->screen;
    const struct AntWalk *w = ant_session_walk(app->session);
    int ox = (s->cols - TRAIL_SIDE * l->grain) / 2;
    int oy = (l->field_rows * 2 - TRAIL_SIDE * l->grain) / 2;
    int row;

    if (oy < 0) {
        oy = 0;
    }
    for (row = 0; row < l->field_rows; ++row) {
        int col;
        bool skipped = false;
        tui_move(s, row, 0);
        for (col = 0; col < s->cols; ++col) {
            /* Под меню поле не рисуется: иначе меню мерцало бы. */
            if (tui_masked(s, row, col)) {
                skipped = true;
                continue;
            }
            if (skipped) {
                tui_move(s, row, col);
                skipped = false;
            }
            tui_pixels(s, pixel_color(w, col, row * 2, ox, oy, l->grain),
                       pixel_color(w, col, row * 2 + 1, ox, oy, l->grain));
        }
    }
    tui_plain(s);
}

/* --- таблица автомата -------------------------------------------------------- */

static void draw_transition(struct App *app, const struct AntFsm *fsm, const struct AntFsm *base,
                            int state, int input) {
    struct TuiScreen *s = &app->screen;
    char text[16];
    int changed = base != NULL && ant_fsm_diff(base, fsm, state, input) != ANT_DIFF_SAME;

    tui_colors(s, COLOR_TABLE, TUI_COLOR_DEFAULT);
    tui_emit(s, input ? "●" : "○");
    ant_transition_text(fsm, state, input, text);
    tui_colors(s, changed ? COLOR_CHANGED : COLOR_TABLE, TUI_COLOR_DEFAULT);
    tui_emit_fitted(s, text, 4);
}

static void draw_panel(struct App *app, const struct Layout *l) {
    struct TuiScreen *s = &app->screen;
    const struct AntSession *session = app->session;
    const struct AntFsm *fsm = panel_fsm(app);
    const struct AntFsm *base = NULL;
    const struct AntWalk *w = ant_session_walk(session);
    char text[512];
    char run[64];
    int i;
    int shown;

    /* С чем сравнивать: кандидата — с показанным автоматом, показанный — с
       тем, что был до него. */
    if (app->pending) {
        base = &session->fsm;
    } else if (session->has_previous) {
        base = &session->previous;
    }

    tui_move(s, l->panel_row, 0);
    if (app->pending) {
        describe_run(&app->candidate_run, run);
        sprintf(text,
                " Кандидат: %d %s, %s, изменено переходов: %d │ Enter — принять  n — отклонить"
                "  a — принимать без вопросов",
                fsm->state_count, plural(fsm->state_count, "состояние", "состояния", "состояний"),
                run, ant_fsm_diff_count(base, fsm));
        tui_colors(s, COLOR_CHANGED, TUI_COLOR_DEFAULT);
    } else if (fsm->state_count > 0) {
        sprintf(text,
                " Автомат: %d %s │ ● еда впереди  ○ пусто │ Ш шаг  Л налево  П направо"
                " │ жёлтым — изменения%s",
                fsm->state_count, plural(fsm->state_count, "состояние", "состояния", "состояний"),
                (base != NULL) ? " к прежнему" : " (сравнивать не с чем)");
        tui_colors(s, COLOR_TABLE, TUI_COLOR_DEFAULT);
    } else {
        sprintf(text, " Автомата нет: o — каталог, r — случайный, s — поиск");
        tui_colors(s, COLOR_TABLE, TUI_COLOR_DEFAULT);
    }
    tui_emit_fitted(s, text, s->cols);

    shown = l->panel_lines * l->columns;
    for (i = 0; i < l->panel_lines * l->columns; ++i) {
        int line = i / l->columns;
        int column = i % l->columns;
        int used;
        if (column == 0) {
            tui_move(s, l->panel_row + 1 + line, 0);
        }
        if (i >= fsm->state_count) {
            tui_plain(s);
            tui_emit_fitted(s, "", CELL_WIDTH);
            continue;
        }
        if (i == l->panel_lines * l->columns - 1 && fsm->state_count > shown) {
            sprintf(text, " …ещё %d", fsm->state_count - shown + 1);
            tui_colors(s, COLOR_TABLE, TUI_COLOR_DEFAULT);
            tui_emit_fitted(s, text, CELL_WIDTH);
            continue;
        }
        /* Текущее состояние прогона — инверсией: видно, какой строкой
           таблицы муравей сейчас руководствуется. */
        sprintf(text, " q%d", i);
        if (!app->pending && i == w->state && fsm->state_count > 0) {
            tui_reverse(s);
            tui_colors(s, COLOR_TABLE, TUI_COLOR_DEFAULT);
        } else {
            tui_plain(s);
            tui_colors(s,
                       (base != NULL && ant_fsm_diff(base, fsm, i, 0) == ANT_DIFF_NEW)
                               ? COLOR_CHANGED
                               : COLOR_TABLE,
                       TUI_COLOR_DEFAULT);
        }
        tui_emit_fitted(s, text, 4);
        tui_plain(s);
        tui_emit(s, " ");
        draw_transition(app, fsm, base, i, 1);
        tui_emit(s, " ");
        draw_transition(app, fsm, base, i, 0);
        used = 4 + 1 + 5 + 1 + 5;
        tui_emit_fitted(s, "", CELL_WIDTH - used);
    }
    tui_plain(s);
}

/* --- строки состояния и поиска ------------------------------------------ */

static void draw_search_line(struct App *app, const struct Layout *l) {
    struct TuiScreen *s = &app->screen;
    char text[512];
    char run[64];
    const char *state;

    if (l->search_row < 0) {
        return;
    }
    if (app->search != NULL) {
        const struct AntSearchResult *r = ant_search_result(app->search);
        describe_run(&r->run, run);
        if (app->pending) {
            state = "ждёт решения";
        } else if (app->auto_accept) {
            state = "идёт, без вопросов";
        } else {
            state = "идёт";
        }
        sprintf(text,
                " Поиск «%s» │ зерно %lu │ оценено %ld │ улучшений %ld │ лучший %s │ принято %ld"
                " │ по %ld за кадр │ %s",
                app->mode->title, app->seed, r->evaluations, r->improvements, run, app->accepted,
                BUDGETS[app->budget_level], state);
    } else {
        sprintf(text, " Поиск «%s» окончен │ зерно %lu │ принято %ld │ s — новый поиск",
                app->mode->title, app->seed, app->accepted);
    }
    tui_move(s, l->search_row, 0);
    tui_colors(s, COLOR_TABLE, TUI_COLOR_DEFAULT);
    tui_emit_fitted(s, text, s->cols);
    tui_plain(s);
}

static void draw_status(struct App *app, const struct Layout *l) {
    struct TuiScreen *s = &app->screen;
    const struct AntSession *session = app->session;
    const struct AntWalk *w = ant_session_walk(session);
    char text[512];

    if (app->message[0] != '\0' && tui_now_ms() < app->message_until) {
        sprintf(text, " %s", app->message);
    } else {
        sprintf(text,
                " %s │ такт %d/%d │ съедено %d/%d │ q%d │ клетка %d │ %d мс │ %s │"
                " p пауза  ←→ такт  b e  +- скорость  ↑↓ клетка  r o s  q выход",
                session->title, w->run.steps, session->steps_limit, w->run.eaten, w->run.total,
                w->state, l->grain, TICKS_MS[app->tick_level],
                app->running ? "идёт" : (ant_session_done(session) ? "конец" : "пауза"));
    }
    tui_move(s, l->status_row, 0);
    tui_reverse(s);
    tui_emit_fitted(s, text, s->cols);
    tui_plain(s);
}

/* --- меню --------------------------------------------------------------- */

static int menu_count(const struct App *app) {
    if (app->overlay == OVERLAY_CATALOG) {
        return ant_preset_count();
    }
    /* В меню поиска первой строкой — остановка, если поиск идёт. */
    return ant_search_mode_count() + (app->search != NULL ? 1 : 0);
}

static void menu_item(const struct App *app, int index, const char **title, const char **group,
                      const char **note) {
    if (app->overlay == OVERLAY_CATALOG) {
        const struct AntPreset *p = ant_preset(index);
        *title = p->title;
        *group = p->group;
        *note = p->note;
        return;
    }
    if (app->search != NULL) {
        if (index == 0) {
            *title = "Остановить поиск";
            *group = "";
            *note = "найденное остаётся на экране";
            return;
        }
        --index;
    }
    *title = ant_search_mode(index)->title;
    *group = (ant_search_mode(index)->algorithm == ANT_SEARCH_EP93) ? "EP93" : "(1+1)-ЭС";
    *note = ant_search_mode(index)->note;
}

/* Где стоит меню: только над полем — таблица автомата и строки состояния
   под ним остаются видны и не перерисовываются поверх. draw_menu его рисует,
   draw_field обходит. false — места для меню нет. */
static bool menu_geometry(const struct App *app, const struct Layout *l, int *top, int *left,
                          int *width, int *lines) {
    const struct TuiScreen *s = &app->screen;
    int count = menu_count(app);

    *width = (s->cols - 4 < 76) ? s->cols - 4 : 76;
    *lines = l->field_rows - 4;
    if (*width < 50 || *lines < 3) {
        return false;
    }
    if (*lines > count) {
        *lines = count;
    }
    *left = (s->cols - *width) / 2;
    *top = (l->field_rows - (*lines + 4)) / 2;
    return true;
}

static void draw_menu(struct App *app, const struct Layout *l) {
    struct TuiScreen *s = &app->screen;
    int width;
    int lines;
    int left;
    int title_w = 36;
    int top;
    int i;
    char text[512];
    const char *title;
    const char *group;
    const char *note;

    if (!menu_geometry(app, l, &top, &left, &width, &lines)) {
        return;
    }
    if (app->menu_index < app->menu_top) {
        app->menu_top = app->menu_index;
    }
    if (app->menu_index >= app->menu_top + lines) {
        app->menu_top = app->menu_index - lines + 1;
    }

    tui_move(s, top, left);
    tui_reverse(s);
    sprintf(text, " %s (%d)   ↑↓ PgUp PgDn — выбор, Enter — %s, Esc — назад",
            (app->overlay == OVERLAY_CATALOG) ? "Автоматы" : "Поиск", menu_count(app),
            (app->overlay == OVERLAY_CATALOG) ? "показать" : "начать");
    tui_emit_fitted(s, text, width);
    tui_plain(s);
    tui_move(s, top + 1, left);
    tui_emit_fitted(s, "", width);
    for (i = 0; i < lines; ++i) {
        int index = app->menu_top + i;
        menu_item(app, index, &title, &group, &note);
        tui_move(s, top + 2 + i, left);
        if (index == app->menu_index) {
            tui_reverse(s);
        }
        sprintf(text, " %2d  ", index + 1);
        tui_emit_fitted(s, text, 5);
        tui_emit_fitted(s, title, title_w);
        tui_emit_fitted(s, group, width - 5 - title_w);
        tui_plain(s);
    }
    tui_move(s, top + 2 + lines, left);
    tui_emit_fitted(s, "", width);
    tui_move(s, top + 3 + lines, left);
    tui_reverse(s);
    menu_item(app, app->menu_index, &title, &group, &note);
    if (app->overlay == OVERLAY_CATALOG) {
        const struct AntPreset *p = ant_preset(app->menu_index);
        sprintf(text, " %s │ съедает %d из 89 за %d %s", note, p->eaten, p->steps,
                plural(p->steps, "такт", "такта", "тактов"));
    } else {
        sprintf(text, " %s", note);
    }
    tui_emit_fitted(s, text, width);
    tui_plain(s);
}

static void draw(struct App *app) {
    struct Layout l;
    int top;
    int left;
    int width;
    int lines;

    if (!tui_frame_begin(&app->screen)) {
        return;
    }
    layout(app, &l);
    if (app->overlay != OVERLAY_NONE && menu_geometry(app, &l, &top, &left, &width, &lines)) {
        tui_set_mask(&app->screen, top, left, lines + 4, width);
    }
    draw_field(app, &l);
    draw_panel(app, &l);
    draw_search_line(app, &l);
    if (app->overlay != OVERLAY_NONE) {
        draw_menu(app, &l);
    }
    draw_status(app, &l);
    tui_frame_end(&app->screen);
}

/* --- поведение -------------------------------------------------------- */

static void show(struct App *app, const struct AntFsm *fsm, int limit, const char *title) {
    ant_session_show(app->session, fsm, limit, title);
    app->running = true;
    app->next_tick = tui_now_ms() + TICKS_MS[app->tick_level];
}

static void load_preset(struct App *app, int index) {
    const struct AntPreset *p = ant_preset(index);
    struct AntFsm fsm;

    if (!ant_preset_fsm(p, &fsm)) {
        say(app, "запись каталога не разбирается");
        return;
    }
    show(app, &fsm, p->steps_limit, p->title);
}

static void stop_search(struct App *app) {
    ant_search_end(app->search);
    app->search = NULL;
    app->pending = false;
}

static void on_improved(const struct AntFsm *fsm, const struct TrailRun *run, long iteration,
                        void *user) {
    struct App *app = (struct App *)user;

    (void)iteration;
    /* Пока человек думает, поиск стоит, поэтому здесь копится только
       последнее — лучшее — из найденного за один кусок поиска. */
    app->candidate = *fsm;
    app->candidate_run = *run;
    app->pending = true;
    ++app->candidate_found;
}

static void accept_candidate(struct App *app) {
    char title[ANT_SESSION_TITLE_SIZE];
    char run[64];

    describe_run(&app->candidate_run, run);
    ++app->accepted;
    sprintf(title, "Поколение %ld: %s", app->accepted, run);
    show(app, &app->candidate, app->candidate_limit, title);
    app->pending = false;
    app->candidate_found = 0;
}

static void start_search(struct App *app, const struct AntSearchMode *mode) {
    struct AntSearch cfg;
    struct AntFsm start;
    int has_start = 0;
    struct AntSearchLog log;

    if (app->search != NULL) {
        stop_search(app);
    }
    /* Зерно своё у каждого поиска и печатается: повторить найденное можно
       командой 11-ant-search с тем же зерном. */
    app->seed = (unsigned long)time(NULL) % 100000UL + (unsigned long)app->accepted;
    ant_search_mode_config(mode, (uint32_t)app->seed, &app->session->fsm, &cfg, &start, &has_start);
    log.improved = on_improved;
    log.user = app;
    app->pending = false;
    app->candidate_found = 0;
    app->accepted = 0;
    app->finished = false;
    app->search = ant_search_begin(&cfg, has_start ? &start : NULL, &log);
    if (app->search == NULL) {
        say(app, "поиск не начат: нет памяти");
        return;
    }
    app->mode = mode;
    app->candidate_limit = cfg.steps_limit;
    if (has_start) {
        /* Начальный автомат и есть нулевое поколение: с ним сравнивается
           первое найденное. */
        app->pending = false;
        show(app, &start, cfg.steps_limit, "Поколение 0: начальный автомат");
        ant_session_forget_previous(app->session);
    } else if (!app->pending) {
        /* (1+1)-ЭС с нуля: случайный начальный автомат — нулевое поколение. */
        show(app, &ant_search_result(app->search)->best, cfg.steps_limit,
             "Поколение 0: случайный автомат");
        ant_session_forget_previous(app->session);
    } else {
        /* У EP93 лучший из начальной популяции найден при её заведении — он
           и ждёт решения. */
        say(app, "поиск начат со случайной популяции: лучший из неё ждёт решения");
    }
}

static void search_frame(struct App *app) {
    struct AntSearchLog log;
    bool done;

    if (app->search == NULL || (app->pending && !app->auto_accept)) {
        return;
    }
    log.improved = on_improved;
    log.user = app;
    done = ant_search_advance(app->search, BUDGETS[app->budget_level], &log);
    if (app->pending && app->auto_accept) {
        accept_candidate(app);
    }
    if (done) {
        const struct AntSearchResult *r = ant_search_result(app->search);
        char text[256];
        char run[64];
        describe_run(&r->run, run);
        sprintf(text, "поиск окончен: оценено %ld, лучший %s", r->evaluations, run);
        say(app, text);
        /* Найденное в последнем куске не теряется: решение по нему за
           человеком, даже когда поиск уже закрыт. */
        ant_search_end(app->search);
        app->search = NULL;
        app->finished = true;
    }
}

static void random_automaton(struct App *app) {
    struct AntFsm fsm;
    int states = 3 + ant_random_below(&app->random, 6);
    char title[64];

    ant_fsm_random(&fsm, states, &app->random);
    sprintf(title, "Случайный: %d %s", states,
            plural(states, "состояние", "состояния", "состояний"));
    show(app, &fsm, TRAIL_STEPS, title);
}

static void handle_menu_key(struct App *app, int key) {
    int count = menu_count(app);
    int page = app->screen.rows / 2;

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
        if (app->overlay == OVERLAY_CATALOG) {
            load_preset(app, app->menu_index);
        } else if (app->search != NULL && app->menu_index == 0) {
            stop_search(app);
            app->finished = true;
            say(app, "поиск остановлен");
        } else {
            start_search(app, ant_search_mode(app->menu_index - (app->search != NULL ? 1 : 0)));
        }
        app->overlay = OVERLAY_NONE;
        break;
    case TUI_KEY_ESC:
    case 'o':
    case 's':
        app->overlay = OVERLAY_NONE;
        break;
    case 'q':
        tui_request_stop();
        break;
    default:
        break;
    }
}

static void open_menu(struct App *app, enum Overlay overlay) {
    app->overlay = overlay;
    app->menu_index = 0;
    app->menu_top = 0;
}

static void handle_key(struct App *app, int key) {
    if (app->overlay != OVERLAY_NONE) {
        handle_menu_key(app, key);
        return;
    }
    switch (key) {
    case 'q':
    case 'Q':
        tui_request_stop();
        break;
    case 'p':
    case 'P':
    case ' ':
        if (ant_session_done(app->session)) {
            ant_session_to_start(app->session);
        }
        app->running = !app->running;
        break;
    case TUI_KEY_RIGHT:
        app->running = false;
        if (!ant_session_forward(app->session)) {
            say(app, "прогон окончен: дальше тактов нет");
        }
        break;
    case TUI_KEY_LEFT:
        app->running = false;
        if (!ant_session_back(app->session)) {
            say(app, "это начало прогона");
        }
        break;
    case 'b':
        app->running = false;
        ant_session_to_start(app->session);
        break;
    case 'e':
        app->running = false;
        ant_session_to_end(app->session);
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
    case ']':
        if (app->budget_level < BUDGET_LEVELS - 1) {
            ++app->budget_level;
        }
        break;
    case '[':
        if (app->budget_level > 0) {
            --app->budget_level;
        }
        break;
    case TUI_KEY_UP:
        if (app->grain < max_grain(app)) {
            ++app->grain;
        }
        break;
    case TUI_KEY_DOWN:
        /* Клетка могла быть ограничена окном: уменьшать от видимой. */
        if (app->grain > max_grain(app)) {
            app->grain = max_grain(app);
        }
        if (app->grain > 1) {
            --app->grain;
        }
        break;
    case 'r':
    case 'R':
        random_automaton(app);
        break;
    case 'o':
    case 'O':
        open_menu(app, OVERLAY_CATALOG);
        break;
    case 's':
    case 'S':
        open_menu(app, OVERLAY_SEARCH);
        break;
    case 'a':
    case 'A':
        app->auto_accept = !app->auto_accept;
        say(app, app->auto_accept ? "найденное принимается без вопросов"
                                  : "каждое улучшение ждёт подтверждения");
        if (app->auto_accept && app->pending) {
            accept_candidate(app);
        }
        break;
    case TUI_KEY_ENTER:
    case 'y':
        if (app->pending) {
            accept_candidate(app);
        }
        break;
    case 'n':
    case 'N':
        if (app->pending) {
            /* Отклонённый кандидат не забывается поиском: он остаётся его
               лучшим, и следующий будет лучше него. */
            app->pending = false;
            app->candidate_found = 0;
            say(app, "кандидат отклонён, поиск продолжается");
        }
        break;
    default:
        break;
    }
}

static void run(struct App *app) {
    app->next_tick = tui_now_ms() + TICKS_MS[app->tick_level];

    while (!tui_stop_requested()) {
        int cols;
        int rows;
        int keys[16];
        int count;
        int i;
        long timeout;
        long now;

        tui_size(&cols, &rows);
        if (cols != app->screen.cols || rows != app->screen.rows) {
            app->screen.cols = cols;
            app->screen.rows = rows;
            tui_clear();
        }
        draw(app);

        now = tui_now_ms();
        timeout = app->running ? app->next_tick - now : IDLE_MS;
        if (app->search != NULL && !(app->pending && !app->auto_accept) && timeout > BUSY_MS) {
            timeout = BUSY_MS;
        }
        count = tui_read_keys(timeout, keys, 16);
        for (i = 0; i < count; ++i) {
            handle_key(app, keys[i]);
        }
        if (app->overlay == OVERLAY_NONE) {
            search_frame(app);
        }
        now = tui_now_ms();
        if (app->running && now >= app->next_tick) {
            if (!ant_session_forward(app->session)) {
                app->running = false;
            }
            app->next_tick = now + TICKS_MS[app->tick_level];
        }
    }
}

int main(void) {
    struct App app;
    struct AntFsm scan;

    if (!tui_is_terminal()) {
        fprintf(stderr, "ant-tui: нужен терминал — программа интерактивная\n");
        return 1;
    }
    memset(&app, 0, sizeof(app));
    app.session = ant_session_create();
    if (app.session == NULL) {
        fprintf(stderr, "ant-tui: не хватило памяти\n");
        return 1;
    }
    app.grain = 1000; /* наибольшая, какая влезет */
    app.tick_level = 3;
    app.budget_level = 1;
    ant_random_init(&app.random, (uint32_t)time(NULL));
    ant_fsm_scan(&scan);
    show(&app, &scan, TRAIL_STEPS, "Сканер: начальный, 315 тактов");

    tui_install_signals();
    if (!tui_enter()) {
        fprintf(stderr, "ant-tui: терминал не переводится в посимвольный режим\n");
        ant_session_destroy(app.session);
        return 1;
    }
    tui_size(&app.screen.cols, &app.screen.rows);

    run(&app);

    tui_leave();
    tui_screen_free(&app.screen);
    if (app.search != NULL) {
        ant_search_end(app.search);
    }
    ant_session_destroy(app.session);
    return 0;
}
