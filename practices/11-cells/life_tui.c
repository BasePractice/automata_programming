/*
 * Лекция 11. Игра «Жизнь» в терминале: поле замкнуто в тор, клетка
 * масштабируется от одного «пикселя» (половина знакоместа) и больше, есть
 * пауза, шаг вперёд и назад, случайная расстановка и каталог готовых
 * объектов — из лекции и из библиотеки LifeWiki и Golly.
 *
 *   r        случайная расстановка          p      пауза и пуск
 *   ← →      шаг назад и вперёд             ↑ ↓    клетка крупнее и мельче
 *   o        каталог объектов               q      выход
 *
 * Здесь только терминал: всё, что программа умеет делать с полем, живёт в
 * life_session.c и проверяется тестом.
 *
 * Программа не входит в обязательную сборку (STATECRAFT_BUILD_TUI):
 * терминальный слой (tui_term.c) построен на termios и ioctl — это POSIX, а
 * не ISO C90, и на Windows их нет.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "life_session.h"
#include "tui_term.h"

#define TICK_MS 80        /* пауза между поколениями при пуске */
#define IDLE_MS 250       /* как часто проверять размер окна в паузе */
#define RANDOM_PERCENT 30 /* доля живых клеток при случайной расстановке */
#define MAX_GRAIN 32      /* самая крупная клетка, в пикселях */
#define MESSAGE_MS 2500   /* сколько держится сообщение в строке состояния */

struct App {
    struct LifeSession *session;
    struct TuiScreen screen;
    int grain;   /* сторона клетки в пикселях; пиксель — половина знакоместа */
    int field_w; /* поле, которое помещается в окно при текущей клетке */
    int field_h;
    bool running;
    bool menu;
    int menu_index;
    int menu_top;
    const char *title; /* что сейчас на поле */
    char message[256];
    long message_until;
};

/* Жива ли клетка под пикселем (px, py) экранной сетки. */
static bool pixel_alive(const struct App *app, int px, int py, int ox, int oy) {
    const struct Life *life = &app->session->life;
    int fx = px - ox;
    int fy = py - oy;

    if (fx < 0 || fy < 0) {
        return false;
    }
    fx /= app->grain;
    fy /= app->grain;
    return fx < life->width && fy < life->height && life->cells[fy][fx] != 0;
}

static void draw_field(struct App *app) {
    struct TuiScreen *s = &app->screen;
    const struct Life *life = &app->session->life;
    int field_rows = s->rows - 1;
    /* Поле по центру: оно меньше окна, если упёрлось в LIFE_MAX_SIDE или
       подрезано до кратного трём сценой с агаром. */
    int ox = (s->cols - life->width * app->grain) / 2;
    int oy = (field_rows * 2 - life->height * app->grain) / 2;
    int row;

    for (row = 0; row < field_rows; ++row) {
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
            /* Живая клетка — цвет текста терминала, без единого цвета:
               поле читается и на светлой, и на тёмной теме. */
            tui_pixels(s,
                       pixel_alive(app, col, row * 2, ox, oy) ? TUI_COLOR_DEFAULT : TUI_PIXEL_EMPTY,
                       pixel_alive(app, col, row * 2 + 1, ox, oy) ? TUI_COLOR_DEFAULT
                                                                  : TUI_PIXEL_EMPTY);
        }
    }
}

static void draw_status(struct App *app) {
    struct TuiScreen *s = &app->screen;
    const struct LifeSession *session = app->session;
    char text[512];

    if (app->message[0] != '\0' && tui_now_ms() < app->message_until) {
        sprintf(text, " %s", app->message);
    } else {
        sprintf(text,
                " %s │ поколение %ld │ поле %d×%d │ клетка %d │ %s │ %s │"
                " r случайно  p пауза  ←→ шаг  ↑↓ клетка  o объекты  q выход",
                app->title, session->generation, session->life.width, session->life.height,
                app->grain, (session->rule == LIFE_RULE_PARITY) ? "чётность" : "B3/S23",
                app->running ? "идёт" : "пауза");
    }
    tui_move(s, s->rows - 1, 0);
    tui_reverse(s);
    tui_emit_fitted(s, text, s->cols);
    tui_plain(s);
}

/* Где стоит меню: его рисует draw_menu, а draw_field обходит. false — окно
   слишком мало для меню. */
static bool menu_geometry(const struct App *app, int *top, int *left, int *width, int *lines) {
    const struct TuiScreen *s = &app->screen;
    int count = life_preset_count();

    *width = (s->cols - 4 < 76) ? s->cols - 4 : 76;
    *lines = s->rows - 6; /* строк списка */
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
    int count = life_preset_count();
    int width;
    int lines;
    int left;
    int title_w = 26;
    int top;
    int i;
    char text[512];

    if (!menu_geometry(app, &top, &left, &width, &lines)) {
        return;
    }
    /* Выбранная строка всегда видна: список прокручивается за ней. */
    if (app->menu_index < app->menu_top) {
        app->menu_top = app->menu_index;
    }
    if (app->menu_index >= app->menu_top + lines) {
        app->menu_top = app->menu_index - lines + 1;
    }

    tui_move(s, top, left);
    tui_reverse(s);
    sprintf(text, " Объекты (%d)   ↑↓ PgUp PgDn — выбор, Enter — поставить, Esc — назад", count);
    tui_emit_fitted(s, text, width);
    tui_plain(s);
    tui_move(s, top + 1, left);
    tui_emit_fitted(s, "", width);
    for (i = 0; i < lines; ++i) {
        int index = app->menu_top + i;
        const struct LifePreset *preset = life_preset(index);
        tui_move(s, top + 2 + i, left);
        if (index == app->menu_index) {
            tui_reverse(s);
        }
        /* Ширину столбцов считают знакоместами, а не байтами: у printf
           "%-28s" кириллица заняла бы вдвое больше и обрезалась посреди
           буквы. */
        sprintf(text, " %2d  ", index + 1);
        tui_emit_fitted(s, text, 5);
        tui_emit_fitted(s, preset->title, title_w);
        tui_emit_fitted(s, preset->group, width - 5 - title_w);
        tui_plain(s);
    }
    tui_move(s, top + 2 + lines, left);
    tui_emit_fitted(s, "", width);
    tui_move(s, top + 3 + lines, left);
    tui_reverse(s);
    sprintf(text, " %s", life_preset(app->menu_index)->note);
    tui_emit_fitted(s, text, width);
    tui_plain(s);
}

static void draw(struct App *app) {
    int top;
    int left;
    int width;
    int lines;

    if (!tui_frame_begin(&app->screen)) {
        return;
    }
    if (app->menu && menu_geometry(app, &top, &left, &width, &lines)) {
        tui_set_mask(&app->screen, top, left, lines + 4, width);
    }
    draw_field(app);
    if (app->menu) {
        draw_menu(app);
    }
    draw_status(app);
    tui_frame_end(&app->screen);
}

/* --- поведение ----------------------------------------------------------- */

static void say(struct App *app, const char *text) {
    strncpy(app->message, text, sizeof(app->message) - 1);
    app->message[sizeof(app->message) - 1] = '\0';
    app->message_until = tui_now_ms() + MESSAGE_MS;
}

/* Поле, которое помещается в окно при клетке grain. */
static void field_for(const struct App *app, int grain, int *w, int *h) {
    *w = app->screen.cols / grain;
    *h = (app->screen.rows - 1) * 2 / grain;
    if (*w > LIFE_MAX_SIDE) {
        *w = LIFE_MAX_SIDE;
    }
    if (*h > LIFE_MAX_SIDE) {
        *h = LIFE_MAX_SIDE;
    }
    if (*w < 1) {
        *w = 1;
    }
    if (*h < 1) {
        *h = 1;
    }
}

/*
 * Пересчитать поле под окно и клетку. Поле меняется, только если изменилось
 * то, что от окна зависит: сцена с агаром подрезает поле до кратного трём, и
 * сравнение с текущим размером поля возвращало бы срезанное на каждом кадре.
 */
static void fit_field(struct App *app) {
    int w;
    int h;

    field_for(app, app->grain, &w, &h);
    if (w != app->field_w || h != app->field_h) {
        app->field_w = w;
        app->field_h = h;
        life_session_resize(app->session, w, h);
    }
}

static void load_preset(struct App *app, int index) {
    const struct LifePreset *preset = life_preset(index);
    int min_w;
    int min_h;
    int w;
    int h;
    char text[256];

    life_preset_min_size(preset, &min_w, &min_h);
    /* Объект должен поместиться целиком: иначе он заденет сам себя через
       край тора, и поведение будет не тем, о котором написано в каталоге. */
    field_for(app, app->grain, &w, &h);
    while (app->grain > 1 && (w < min_w || h < min_h)) {
        --app->grain;
        field_for(app, app->grain, &w, &h);
    }
    /* Размер задаётся явно, а не через fit_field: прошлая сцена с агаром
       могла подрезать поле, и его надо вернуть к размеру окна. */
    app->field_w = w;
    app->field_h = h;
    life_session_resize(app->session, w, h);
    if (!life_session_load(app->session, preset)) {
        say(app, "объект не поставлен");
        return;
    }
    app->title = preset->title;
    app->running = false;
    if (w < min_w || h < min_h) {
        sprintf(text, "окно мало для объекта: нужно поле %d×%d — объект задевает себя через край",
                min_w, min_h);
        say(app, text);
    }
}

static void handle_menu_key(struct App *app, int key) {
    int count = life_preset_count();
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
        app->menu = false;
        load_preset(app, app->menu_index);
        break;
    case TUI_KEY_ESC:
    case 'o':
    case 'O':
        app->menu = false;
        break;
    case 'q':
    case 'Q':
        tui_request_stop();
        break;
    default:
        break;
    }
}

static void handle_key(struct App *app, int key) {
    int w;
    int h;

    if (app->menu) {
        handle_menu_key(app, key);
        return;
    }
    switch (key) {
    case 'q':
    case 'Q':
        tui_request_stop();
        break;
    case 'r':
    case 'R':
        life_session_randomize(app->session, RANDOM_PERCENT);
        app->title = "Случайное поле";
        break;
    case 'p':
    case 'P':
    case ' ':
        app->running = !app->running;
        break;
    case 'o':
    case 'O':
        app->menu = true;
        break;
    case TUI_KEY_RIGHT:
        /* Шаг имеет смысл только на остановленном поле: он и останавливает. */
        app->running = false;
        life_session_forward(app->session);
        break;
    case TUI_KEY_LEFT:
        app->running = false;
        if (!life_session_back(app->session)) {
            say(app, (app->session->generation == 0)
                             ? "это начальная расстановка: раньше неё ничего нет"
                             : "назад некуда: история хранит 128 поколений и сбрасывается при "
                               "смене поля");
        }
        break;
    case TUI_KEY_UP:
        field_for(app, app->grain + 1, &w, &h);
        if (app->grain < MAX_GRAIN && w >= 4 && h >= 4) {
            ++app->grain;
            fit_field(app);
        }
        break;
    case TUI_KEY_DOWN:
        if (app->grain > 1) {
            --app->grain;
            fit_field(app);
        }
        break;
    default:
        break;
    }
}

static void run(struct App *app) {
    long next_tick = tui_now_ms() + TICK_MS;

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
            fit_field(app);
        }
        draw(app);

        timeout = (app->running && !app->menu) ? next_tick - tui_now_ms() : IDLE_MS;
        count = tui_read_keys(timeout, keys, 16);
        for (i = 0; i < count; ++i) {
            handle_key(app, keys[i]);
        }
        if (count > 0) {
            continue;
        }
        if (app->running && !app->menu && tui_now_ms() >= next_tick) {
            life_session_forward(app->session);
            next_tick = tui_now_ms() + TICK_MS;
        }
    }
}

int main(void) {
    struct App app;

    if (!tui_is_terminal()) {
        fprintf(stderr, "life-tui: нужен терминал — программа интерактивная\n");
        return 1;
    }
    memset(&app, 0, sizeof(app));
    app.session = life_session_create(1, 1);
    if (app.session == NULL) {
        fprintf(stderr, "life-tui: не хватило памяти под поле\n");
        return 1;
    }
    app.grain = 2;
    app.running = true;
    app.title = "Случайное поле";

    srand((unsigned int)time(NULL));
    tui_install_signals();
    if (!tui_enter()) {
        fprintf(stderr, "life-tui: терминал не переводится в посимвольный режим\n");
        life_session_destroy(app.session);
        return 1;
    }
    tui_size(&app.screen.cols, &app.screen.rows);
    fit_field(&app);
    life_session_randomize(app.session, RANDOM_PERCENT);

    run(&app);

    tui_leave();
    tui_screen_free(&app.screen);
    life_session_destroy(app.session);
    return 0;
}
