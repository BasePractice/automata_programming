/* Макросы тестируемых возможностей — до любого включения: в строгом режиме
   (-std=c90) glibc прячет объявления POSIX. На macOS те же макросы прячут
   struct winsize, и её возвращает _DARWIN_C_SOURCE. */
#define _XOPEN_SOURCE 600
#if defined(__APPLE__)
#define _DARWIN_C_SOURCE
#endif

#include "tui_term.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <termios.h>
#include <unistd.h>

static volatile sig_atomic_t stop_flag = 0;
static struct termios saved_termios;

static void on_signal(int sig) {
    (void)sig;
    stop_flag = 1;
}

void tui_install_signals(void) {
    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);
}

bool tui_stop_requested(void) {
    return stop_flag != 0;
}

void tui_request_stop(void) {
    stop_flag = 1;
}

long tui_now_ms(void) {
    struct timeval tv;

    gettimeofday(&tv, NULL);
    return (long)tv.tv_sec * 1000L + (long)(tv.tv_usec / 1000);
}

bool tui_is_terminal(void) {
    return isatty(STDIN_FILENO) && isatty(STDOUT_FILENO);
}

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

bool tui_enter(void) {
    struct termios raw;

    if (tcgetattr(STDIN_FILENO, &saved_termios) != 0) {
        return false;
    }
    raw = saved_termios;
    /* Без построчного ввода и без эха; ISIG оставлен, чтобы Ctrl+C работал и
       приводил к штатному выходу через флаг остановки. */
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

void tui_leave(void) {
    put("\033[0m\033[?25h\033[?1049l");
    /* TCSANOW, а не TCSAFLUSH: выход не ждёт, пока терминал дочитает вывод. */
    tcsetattr(STDIN_FILENO, TCSANOW, &saved_termios);
}

void tui_size(int *cols, int *rows) {
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0 && ws.ws_row > 0) {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
    } else {
        *cols = 80;
        *rows = 24;
    }
}

void tui_clear(void) {
    put("\033[0m\033[2J");
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
        return TUI_KEY_ENTER;
    }
    if (buf[0] != 27) {
        return buf[0];
    }
    if (len >= 3 && (buf[1] == '[' || buf[1] == 'O')) {
        *used = 3;
        switch (buf[2]) {
        case 'A':
            return TUI_KEY_UP;
        case 'B':
            return TUI_KEY_DOWN;
        case 'C':
            return TUI_KEY_RIGHT;
        case 'D':
            return TUI_KEY_LEFT;
        default:
            break;
        }
        if (len >= 4 && buf[3] == '~') {
            *used = 4;
            if (buf[2] == '5') {
                return TUI_KEY_PAGE_UP;
            }
            if (buf[2] == '6') {
                return TUI_KEY_PAGE_DOWN;
            }
        }
        return TUI_KEY_NONE;
    }
    return TUI_KEY_ESC;
}

int tui_read_keys(long timeout_ms, int *keys, int max) {
    fd_set set;
    struct timeval tv;
    unsigned char buf[64];
    int len;
    int pos = 0;
    int count = 0;

    if (timeout_ms < 0) {
        timeout_ms = 0;
    }
    FD_ZERO(&set);
    FD_SET(STDIN_FILENO, &set);
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    if (select(STDIN_FILENO + 1, &set, NULL, NULL, &tv) <= 0) {
        return 0;
    }
    len = (int)read(STDIN_FILENO, buf, sizeof(buf));
    while (pos < len && count < max) {
        int used;
        int key = parse_key(buf + pos, len - pos, &used);
        if (key != TUI_KEY_NONE) {
            keys[count++] = key;
        }
        pos += used;
    }
    return count;
}

/* --- кадр ---------------------------------------------------------------- */

bool tui_frame_begin(struct TuiScreen *s) {
    /* На знакоместо — до трёх байт UTF-8 и до двух смен цвета по 11 байт,
       плюс запас на меню и строку состояния. */
    size_t need = (size_t)s->cols * (size_t)s->rows * 28 + 8192;

    if (need > s->size) {
        char *frame = (char *)realloc(s->frame, need);
        if (frame == NULL) {
            return false;
        }
        s->frame = frame;
        s->size = need;
    }
    s->used = 0;
    s->mask_rows = 0;
    /* Синхронный вывод (DEC 2026): терминал показывает кадр целиком, когда
       тот дописан, а не по мере прихода байтов. Кадр с цветами — десятки
       килобайт, и без этого видна его верхняя половина поверх старой нижней.
       Терминал, не знающий режима, последовательность пропускает. */
    tui_emit(s, "\033[?2026h");
    /* Цвет на экране после прошлого кадра неизвестен: начинаем со сброса. */
    tui_plain(s);
    return true;
}

void tui_frame_end(struct TuiScreen *s) {
    tui_plain(s);
    tui_emit(s, "\033[?2026l");
    write_all(s->frame, s->used);
}

void tui_set_mask(struct TuiScreen *s, int top, int left, int rows, int cols) {
    s->mask_top = top;
    s->mask_left = left;
    s->mask_rows = rows;
    s->mask_cols = cols;
}

bool tui_masked(const struct TuiScreen *s, int row, int col) {
    return s->mask_rows > 0 && row >= s->mask_top && row < s->mask_top + s->mask_rows
           && col >= s->mask_left && col < s->mask_left + s->mask_cols;
}

void tui_screen_free(struct TuiScreen *s) {
    free(s->frame);
    s->frame = NULL;
    s->size = 0;
    s->used = 0;
}

static void emit_bytes(struct TuiScreen *s, const char *data, size_t len) {
    if (s->used + len < s->size) {
        memcpy(s->frame + s->used, data, len);
        s->used += len;
    }
}

void tui_emit(struct TuiScreen *s, const char *text) {
    emit_bytes(s, text, strlen(text));
}

void tui_move(struct TuiScreen *s, int row, int col) {
    char seq[32];

    sprintf(seq, "\033[%d;%dH", row + 1, col + 1);
    tui_emit(s, seq);
}

void tui_colors(struct TuiScreen *s, int fg, int bg) {
    char seq[32];

    if (fg != s->fg) {
        if (fg == TUI_COLOR_DEFAULT) {
            tui_emit(s, "\033[39m");
        } else {
            sprintf(seq, "\033[38;5;%dm", fg);
            tui_emit(s, seq);
        }
        s->fg = fg;
    }
    if (bg != s->bg) {
        if (bg == TUI_COLOR_DEFAULT) {
            tui_emit(s, "\033[49m");
        } else {
            sprintf(seq, "\033[48;5;%dm", bg);
            tui_emit(s, seq);
        }
        s->bg = bg;
    }
}

void tui_reverse(struct TuiScreen *s) {
    tui_plain(s);
    tui_emit(s, "\033[7m");
}

void tui_plain(struct TuiScreen *s) {
    tui_emit(s, "\033[0m");
    s->fg = TUI_COLOR_DEFAULT;
    s->bg = TUI_COLOR_DEFAULT;
}

int tui_text_width(const char *text) {
    const unsigned char *p = (const unsigned char *)text;
    int width = 0;

    for (; *p != '\0'; ++p) {
        if ((*p & 0xC0) != 0x80) {
            ++width;
        }
    }
    return width;
}

void tui_emit_fitted(struct TuiScreen *s, const char *text, int width) {
    const unsigned char *p = (const unsigned char *)text;
    int shown = 0;

    while (*p != '\0' && shown < width) {
        const unsigned char *start = p;
        ++p;
        while ((*p & 0xC0) == 0x80) {
            ++p;
        }
        emit_bytes(s, (const char *)start, (size_t)(p - start));
        ++shown;
    }
    for (; shown < width; ++shown) {
        tui_emit(s, " ");
    }
}

#define GLYPH_FULL "\342\226\210"   /* полный блок */
#define GLYPH_TOP "\342\226\200"    /* верхняя половина */
#define GLYPH_BOTTOM "\342\226\204" /* нижняя половина */

void tui_pixels(struct TuiScreen *s, int top, int bottom) {
    if (top == TUI_PIXEL_EMPTY && bottom == TUI_PIXEL_EMPTY) {
        tui_colors(s, s->fg, TUI_COLOR_DEFAULT);
        tui_emit(s, " ");
    } else if (top == TUI_PIXEL_EMPTY) {
        tui_colors(s, bottom, TUI_COLOR_DEFAULT);
        tui_emit(s, GLYPH_BOTTOM);
    } else if (bottom == TUI_PIXEL_EMPTY) {
        tui_colors(s, top, TUI_COLOR_DEFAULT);
        tui_emit(s, GLYPH_TOP);
    } else if (top == bottom) {
        tui_colors(s, top, TUI_COLOR_DEFAULT);
        tui_emit(s, GLYPH_FULL);
    } else if (bottom == TUI_COLOR_DEFAULT) {
        /* Цвет текста терминала нельзя поставить фоном — переворачиваем
           глиф: нижняя половина текстом, верхняя фоном. */
        tui_colors(s, bottom, top);
        tui_emit(s, GLYPH_BOTTOM);
    } else {
        tui_colors(s, top, bottom);
        tui_emit(s, GLYPH_TOP);
    }
}
