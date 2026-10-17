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
 * Программа не входит в обязательную сборку (STATECRAFT_BUILD_TUI): режим
 * терминала задаётся через termios и ioctl — это POSIX, а не ISO C90, и на
 * Windows их нет. Библиотеки вроде ncurses не нужны: управляющих
 * последовательностей ANSI достаточно, и они понятны любому эмулятору
 * терминала.
 */

/* Макросы тестируемых возможностей — до любого включения: в строгом режиме
   (-std=c90) glibc прячет объявления POSIX. На macOS те же макросы прячут
   struct winsize, и её возвращает _DARWIN_C_SOURCE. */
#define _XOPEN_SOURCE 600
#if defined(__APPLE__)
#define _DARWIN_C_SOURCE
#endif

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <termios.h>
#include <unistd.h>

#include "life_session.h"

#define TICK_MS 80        /* пауза между поколениями при пуске */
#define IDLE_MS 250       /* как часто проверять размер окна в паузе */
#define RANDOM_PERCENT 30 /* доля живых клеток при случайной расстановке */
#define MAX_GRAIN 32      /* самая крупная клетка, в пикселях */
#define MESSAGE_MS 2500   /* сколько держится сообщение в строке состояния */

enum Key {
    KEY_NONE = 0,
    KEY_ESC = 1000,
    KEY_UP,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_PAGE_UP,
    KEY_PAGE_DOWN,
    KEY_ENTER
};

struct Screen {
    int cols;
    int rows;
    char *frame; /* кадр целиком: выводится одной записью, без мерцания */
    size_t size;
    size_t used;
};

struct App {
    struct LifeSession *session;
    struct Screen screen;
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

static volatile sig_atomic_t stop_requested = 0;
static struct termios saved_termios;

static void on_signal(int sig) {
    (void)sig;
    stop_requested = 1;
}

static long now_ms(void) {
    struct timeval tv;

    gettimeofday(&tv, NULL);
    return (long)tv.tv_sec * 1000L + (long)(tv.tv_usec / 1000);
}

/* --- терминал ------------------------------------------------------------ */

static void write_all(const char *data, size_t size) {
    while (size > 0) {
        ssize_t n = write(STDOUT_FILENO, data, size);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return;
        }
        data += n;
        size -= (size_t)n;
    }
}

static void put(const char *text) {
    write_all(text, strlen(text));
}

static bool terminal_enter(void) {
    struct termios raw;

    if (tcgetattr(STDIN_FILENO, &saved_termios) != 0) {
        return false;
    }
    raw = saved_termios;
    /* Без построчного ввода и без эха; ISIG оставлен, чтобы Ctrl+C работал и
       приводил к штатному выходу через on_signal. */
    raw.c_lflag &= ~(tcflag_t)(ICANON | ECHO);
    raw.c_iflag &= ~(tcflag_t)(IXON | ICRNL);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) != 0) {
        return false;
    }
    /* Отдельный экран, скрытый курсор: по выходе терминал вернётся к тому,
       что было до запуска. */
    put("\033[?1049h\033[?25l\033[2J");
    return true;
}

static void terminal_leave(void) {
    put("\033[0m\033[?25h\033[?1049l");
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved_termios);
}

static void terminal_size(int *cols, int *rows) {
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0 && ws.ws_row > 0) {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
    } else {
        *cols = 80;
        *rows = 24;
    }
}

/* Ждать ввод не дольше timeout_ms; true — есть что читать. */
static bool wait_input(long timeout_ms) {
    fd_set set;
    struct timeval tv;

    if (timeout_ms < 0) {
        timeout_ms = 0;
    }
    FD_ZERO(&set);
    FD_SET(STDIN_FILENO, &set);
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    return select(STDIN_FILENO + 1, &set, NULL, NULL, &tv) > 0;
}

/*
 * Разбор нажатий. Стрелки и листание приходят последовательностями
 * ESC [ A … ESC [ D, ESC [ 5 ~, ESC [ 6 ~ (или ESC O A в режиме приложения).
 * Одиночный ESC — это сама клавиша Esc: всё, что пришло одним чтением,
 * разбирается целиком, и если за ESC ничего нет, продолжения не будет.
 */
static int parse_key(const unsigned char *buf, int len, int *used) {
    *used = 1;
    if (buf[0] == '\r' || buf[0] == '\n') {
        return KEY_ENTER;
    }
    if (buf[0] != 27) {
        return buf[0];
    }
    if (len >= 3 && (buf[1] == '[' || buf[1] == 'O')) {
        *used = 3;
        switch (buf[2]) {
        case 'A':
            return KEY_UP;
        case 'B':
            return KEY_DOWN;
        case 'C':
            return KEY_RIGHT;
        case 'D':
            return KEY_LEFT;
        default:
            break;
        }
        if (len >= 4 && buf[3] == '~') {
            *used = 4;
            if (buf[2] == '5') {
                return KEY_PAGE_UP;
            }
            if (buf[2] == '6') {
                return KEY_PAGE_DOWN;
            }
        }
        return KEY_NONE;
    }
    return KEY_ESC;
}

/* --- кадр ---------------------------------------------------------------- */

static bool screen_reserve(struct Screen *s) {
    /* На знакоместо — до трёх байт UTF-8 и запас на управляющие
       последовательности строк, меню и строки состояния. */
    size_t need = (size_t)s->cols * (size_t)s->rows * 8 + 4096;

    if (need > s->size) {
        char *frame = (char *)realloc(s->frame, need);
        if (frame == NULL) {
            return false;
        }
        s->frame = frame;
        s->size = need;
    }
    s->used = 0;
    return true;
}

static void emit(struct Screen *s, const char *text) {
    size_t len = strlen(text);

    if (s->used + len < s->size) {
        memcpy(s->frame + s->used, text, len);
        s->used += len;
    }
}

static void move_to(struct Screen *s, int row, int col) {
    char seq[32];

    sprintf(seq, "\033[%d;%dH", row + 1, col + 1);
    emit(s, seq);
}

/* Вывести text ровно в width знакомест: длиннее — обрезать, короче —
   дополнить пробелами. Знакоместо — один символ UTF-8: в программе нет
   символов двойной ширины. */
static void emit_fitted(struct Screen *s, const char *text, int width) {
    const unsigned char *p = (const unsigned char *)text;
    int shown = 0;

    while (*p != '\0' && shown < width) {
        const unsigned char *start = p;
        size_t len;
        ++p;
        while ((*p & 0xC0) == 0x80) {
            ++p;
        }
        len = (size_t)(p - start);
        if (s->used + len < s->size) {
            memcpy(s->frame + s->used, start, len);
            s->used += len;
        }
        ++shown;
    }
    for (; shown < width; ++shown) {
        emit(s, " ");
    }
}

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
    struct Screen *s = &app->screen;
    const struct Life *life = &app->session->life;
    int field_rows = s->rows - 1;
    /* Поле по центру: оно меньше окна, если упёрлось в LIFE_MAX_SIDE или
       подрезано до кратного трём сценой с агаром. */
    int ox = (s->cols - life->width * app->grain) / 2;
    int oy = (field_rows * 2 - life->height * app->grain) / 2;
    int row;

    for (row = 0; row < field_rows; ++row) {
        int col;
        move_to(s, row, 0);
        for (col = 0; col < s->cols; ++col) {
            bool top = pixel_alive(app, col, row * 2, ox, oy);
            bool bottom = pixel_alive(app, col, row * 2 + 1, ox, oy);
            /* Знакоместо примерно вдвое выше своей ширины, поэтому в нём два
               квадратных пикселя: верхняя и нижняя половины блока. */
            if (top && bottom) {
                emit(s, "\342\226\210"); /* полный блок */
            } else if (top) {
                emit(s, "\342\226\200"); /* верхняя половина */
            } else if (bottom) {
                emit(s, "\342\226\204"); /* нижняя половина */
            } else {
                emit(s, " ");
            }
        }
    }
}

static void draw_status(struct App *app) {
    struct Screen *s = &app->screen;
    const struct LifeSession *session = app->session;
    char text[512];

    if (app->message[0] != '\0' && now_ms() < app->message_until) {
        sprintf(text, " %s", app->message);
    } else {
        sprintf(text,
                " %s │ поколение %ld │ поле %d×%d │ клетка %d │ %s │ %s │"
                " r случайно  p пауза  ←→ шаг  ↑↓ клетка  o объекты  q выход",
                app->title, session->generation, session->life.width, session->life.height,
                app->grain, (session->rule == LIFE_RULE_PARITY) ? "чётность" : "B3/S23",
                app->running ? "идёт" : "пауза");
    }
    move_to(s, s->rows - 1, 0);
    emit(s, "\033[7m");
    emit_fitted(s, text, s->cols);
    emit(s, "\033[0m");
}

static void draw_menu(struct App *app) {
    struct Screen *s = &app->screen;
    int count = life_preset_count();
    int width = (s->cols - 4 < 76) ? s->cols - 4 : 76;
    int lines = s->rows - 6; /* строк списка */
    int left = (s->cols - width) / 2;
    int title_w = 26;
    int top;
    int i;
    char text[512];

    if (width < 40 || lines < 3) {
        return;
    }
    if (lines > count) {
        lines = count;
    }
    /* Выбранная строка всегда видна: список прокручивается за ней. */
    if (app->menu_index < app->menu_top) {
        app->menu_top = app->menu_index;
    }
    if (app->menu_index >= app->menu_top + lines) {
        app->menu_top = app->menu_index - lines + 1;
    }
    top = (s->rows - 1 - (lines + 4)) / 2;

    move_to(s, top, left);
    emit(s, "\033[7m");
    sprintf(text, " Объекты (%d)   ↑↓ PgUp PgDn — выбор, Enter — поставить, Esc — назад", count);
    emit_fitted(s, text, width);
    emit(s, "\033[0m");
    move_to(s, top + 1, left);
    emit_fitted(s, "", width);
    for (i = 0; i < lines; ++i) {
        int index = app->menu_top + i;
        const struct LifePreset *preset = life_preset(index);
        move_to(s, top + 2 + i, left);
        if (index == app->menu_index) {
            emit(s, "\033[7m");
        }
        /* Ширину столбцов считают знакоместами, а не байтами: у printf
           "%-28s" кириллица заняла бы вдвое больше и обрезалась посреди
           буквы. */
        sprintf(text, " %2d  ", index + 1);
        emit_fitted(s, text, 5);
        emit_fitted(s, preset->title, title_w);
        emit_fitted(s, preset->group, width - 5 - title_w);
        emit(s, "\033[0m");
    }
    move_to(s, top + 2 + lines, left);
    emit_fitted(s, "", width);
    move_to(s, top + 3 + lines, left);
    emit(s, "\033[7m");
    sprintf(text, " %s", life_preset(app->menu_index)->note);
    emit_fitted(s, text, width);
    emit(s, "\033[0m");
}

static void draw(struct App *app) {
    if (!screen_reserve(&app->screen)) {
        return;
    }
    draw_field(app);
    if (app->menu) {
        draw_menu(app);
    }
    draw_status(app);
    write_all(app->screen.frame, app->screen.used);
}

/* --- поведение ----------------------------------------------------------- */

static void say(struct App *app, const char *text) {
    strncpy(app->message, text, sizeof(app->message) - 1);
    app->message[sizeof(app->message) - 1] = '\0';
    app->message_until = now_ms() + MESSAGE_MS;
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
    case KEY_UP:
        app->menu_index = (app->menu_index + count - 1) % count;
        break;
    case KEY_DOWN:
        app->menu_index = (app->menu_index + 1) % count;
        break;
    case KEY_PAGE_UP:
        app->menu_index = (app->menu_index > page) ? app->menu_index - page : 0;
        break;
    case KEY_PAGE_DOWN:
        app->menu_index = (app->menu_index + page < count) ? app->menu_index + page : count - 1;
        break;
    case KEY_ENTER:
        app->menu = false;
        load_preset(app, app->menu_index);
        break;
    case KEY_ESC:
    case 'o':
    case 'O':
        app->menu = false;
        break;
    case 'q':
    case 'Q':
        stop_requested = 1;
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
        stop_requested = 1;
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
    case KEY_RIGHT:
        /* Шаг имеет смысл только на остановленном поле: он и останавливает. */
        app->running = false;
        life_session_forward(app->session);
        break;
    case KEY_LEFT:
        app->running = false;
        if (!life_session_back(app->session)) {
            say(app, (app->session->generation == 0)
                             ? "это начальная расстановка: раньше неё ничего нет"
                             : "назад некуда: история хранит 128 поколений и сбрасывается при "
                               "смене поля");
        }
        break;
    case KEY_UP:
        field_for(app, app->grain + 1, &w, &h);
        if (app->grain < MAX_GRAIN && w >= 4 && h >= 4) {
            ++app->grain;
            fit_field(app);
        }
        break;
    case KEY_DOWN:
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
    long next_tick = now_ms() + TICK_MS;
    unsigned char buf[64];

    while (!stop_requested) {
        int cols;
        int rows;
        long timeout;

        terminal_size(&cols, &rows);
        if (cols != app->screen.cols || rows != app->screen.rows) {
            app->screen.cols = cols;
            app->screen.rows = rows;
            put("\033[2J");
            fit_field(app);
        }
        draw(app);

        timeout = (app->running && !app->menu) ? next_tick - now_ms() : IDLE_MS;
        if (wait_input(timeout)) {
            int len = (int)read(STDIN_FILENO, buf, sizeof(buf));
            int pos = 0;
            while (pos < len) {
                int used;
                int key = parse_key(buf + pos, len - pos, &used);
                handle_key(app, key);
                pos += used;
            }
            continue;
        }
        if (app->running && !app->menu && now_ms() >= next_tick) {
            life_session_forward(app->session);
            next_tick = now_ms() + TICK_MS;
        }
    }
}

int main(void) {
    struct App app;

    if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO)) {
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
    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);
    if (!terminal_enter()) {
        fprintf(stderr, "life-tui: терминал не переводится в посимвольный режим\n");
        life_session_destroy(app.session);
        return 1;
    }
    terminal_size(&app.screen.cols, &app.screen.rows);
    fit_field(&app);
    life_session_randomize(app.session, RANDOM_PERCENT);

    run(&app);

    terminal_leave();
    free(app.screen.frame);
    life_session_destroy(app.session);
    return 0;
}
