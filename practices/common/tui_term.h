#ifndef STATECRAFT_TUI_TERM_H
#define STATECRAFT_TUI_TERM_H

/**
 * @file
 * Лекции 10 и 11. Общий терминальный слой программ-демонстраторов mt_tui
 * (машина Тьюринга, практика 10-turing-machine), life_tui (игра «Жизнь»,
 * 11-cells) и ant_tui (умный муравей, 11-ant-search).
 *
 * Только POSIX и управляющие последовательности ANSI: посимвольный ввод
 * через termios, размер окна через ioctl, кадр собирается в памяти и
 * выводится одной записью — так экран не мерцает. Библиотеки вроде ncurses
 * не нужны. На Windows этого нет, поэтому слой собирается только вместе с
 * программами, под опцией STATECRAFT_BUILD_TUI.
 *
 * Пиксель — половина знакоместа: знакоместо примерно вдвое выше своей
 * ширины, и символ «верхняя половина блока» с цветом текста для верхнего
 * пикселя и цветом фона для нижнего даёт два квадратных пикселя.
 */

#include <stddef.h>

#include "base_types.h"

#if defined(__cplusplus)
extern "C" {
#endif

/** Клавиши, которые приходят многобайтовыми последовательностями. Обычные
    символы возвращаются своим кодом (0..255). */
enum TuiKey {
    TUI_KEY_NONE = 0,
    TUI_KEY_ESC = 1000,
    TUI_KEY_UP,
    TUI_KEY_DOWN,
    TUI_KEY_LEFT,
    TUI_KEY_RIGHT,
    TUI_KEY_PAGE_UP,
    TUI_KEY_PAGE_DOWN,
    TUI_KEY_ENTER
};

/** Цвет «по умолчанию терминала»: без управляющей последовательности. */
#define TUI_COLOR_DEFAULT (-1)

/** Кадр в памяти. Поля cols и rows заполняет вызывающий (tui_size). */
struct TuiScreen {
    int cols;
    int rows;
    char *frame;
    size_t size;
    size_t used;
    int fg;       /**< цвет текста, который уже выставлен в кадре */
    int bg;       /**< цвет фона, который уже выставлен в кадре */
    int mask_top; /**< прямоугольник, который рисует кто-то другой (меню) */
    int mask_left;
    int mask_rows; /**< 0 — прямоугольника нет */
    int mask_cols;
};

/** Ctrl+C и завершение процесса ставят флаг, а не обрывают программу: иначе
    терминал остался бы в посимвольном режиме. */
void tui_install_signals(void);
bool tui_stop_requested(void);
void tui_request_stop(void);

/** Миллисекунды от произвольной точки отсчёта. */
long tui_now_ms(void);

/** true, если и ввод, и вывод — терминал. */
bool tui_is_terminal(void);

/** Посимвольный ввод без эха, отдельный экран, скрытый курсор.
    false — терминал в такой режим не переводится. */
bool tui_enter(void);

/** Вернуть терминал в исходное состояние; вызывать обязательно. */
void tui_leave(void);

/** Размер окна в знакоместах; если он не известен — 80 x 24. */
void tui_size(int *cols, int *rows);

/** Очистить весь экран (после изменения размера окна). */
void tui_clear(void);

/**
 * Ждать нажатий не дольше @p timeout_ms и разобрать их.
 *
 * @return сколько клавиш записано в @p keys (не больше @p max); 0 — истекло
 *         время ожидания.
 */
int tui_read_keys(long timeout_ms, int *keys, int max);

/** Подготовить пустой кадр под текущие cols x rows. false — нет памяти. */
bool tui_frame_begin(struct TuiScreen *s);

/**
 * Прямоугольник поверх поля — меню. Поле под ним не рисуется вовсе: иначе в
 * каждом кадре терминал сначала показывал бы поле, а затем меню поверх, и
 * меню мерцало бы. tui_frame_begin прямоугольник снимает.
 */
void tui_set_mask(struct TuiScreen *s, int top, int left, int rows, int cols);

/** Лежит ли знакоместо под прямоугольником tui_set_mask. */
bool tui_masked(const struct TuiScreen *s, int row, int col);

/** Вывести собранный кадр одной записью. */
void tui_frame_end(struct TuiScreen *s);

void tui_screen_free(struct TuiScreen *s);

/** Добавить в кадр текст как есть (в том числе управляющие
    последовательности). */
void tui_emit(struct TuiScreen *s, const char *text);

/** Курсор в строку @p row и столбец @p col, считая от нуля. */
void tui_move(struct TuiScreen *s, int row, int col);

/** Цвета текста и фона по палитре из 256 цветов либо #TUI_COLOR_DEFAULT.
    Последовательность выводится, только если цвет меняется. */
void tui_colors(struct TuiScreen *s, int fg, int bg);

/** Инверсия (строки меню и состояния) и сброс всех атрибутов. */
void tui_reverse(struct TuiScreen *s);
void tui_plain(struct TuiScreen *s);

/**
 * Вывести @p text ровно в @p width знакомест: длиннее — обрезать, короче —
 * дополнить пробелами. Знакоместо — один символ UTF-8: символов двойной
 * ширины программы не выводят.
 */
void tui_emit_fitted(struct TuiScreen *s, const char *text, int width);

/** Число знакомест в строке UTF-8. */
int tui_text_width(const char *text);

/**
 * Знакоместо из двух пикселей: верхнего цвета @p top и нижнего цвета
 * @p bottom. Цвет -2 означает «пусто» (фон терминала), #TUI_COLOR_DEFAULT —
 * цвет текста терминала: так «Жизнь» рисуется без единого цвета и читается
 * на любой теме.
 */
void tui_pixels(struct TuiScreen *s, int top, int bottom);

/** «Пусто» для tui_pixels. */
#define TUI_PIXEL_EMPTY (-2)

#if defined(__cplusplus)
}
#endif

#endif /* STATECRAFT_TUI_TERM_H */
