#ifndef STATECRAFT_CELLS_H
#define STATECRAFT_CELLS_H

/**
 * @file
 * Лекция 11. Три клеточных автомата, на которых видно, что простое правило
 * не означает простого поведения:
 *
 *   elementary — одномерный автомат Вольфрама: 2 состояния, окрестность из
 *                трёх клеток, правило задаётся числом 0..255;
 *   life       — игра «Жизнь» Конуэя: двумерная окрестность Мура, три
 *                правила рождения и выживания;
 *   ant        — муравей Лэнгтона: автомат из двух правил, который сначала
 *                хаотичен, а после ~10 000 шагов строит периодическое «шоссе».
 *
 * Поле везде конечное. Это не приближение к бесконечной решётке из
 * определения, а сознательное ограничение: у конечного поля есть край, и
 * поведение у края отличается от поведения в середине. Где это существенно,
 * сказано в комментарии к функции.
 */

#include <stdio.h>

#include "base_types.h"

#if defined(__cplusplus)
extern "C" {
#endif

/* --- одномерный автомат Вольфрама ---------------------------------------- */

#define CELLS_MAX_WIDTH 256 /**< клеток в одномерном поле */

/**
 * Правило нумеруется по Вольфраму: восемь возможных окрестностей
 * (левый, центр, правый) упорядочены как двоичные числа 111, 110, …, 000, и
 * новое состояние центральной клетки для каждой из них — соответствующий бит
 * числа rule. Отсюда ровно 256 элементарных автоматов.
 */
struct Elementary {
    unsigned char rule; /**< номер правила, 0..255 */
    int width;          /**< длина поля */
    char cells[CELLS_MAX_WIDTH];
};

/**
 * Начальное состояние — одна живая клетка посередине.
 *
 * @param ca    поле, которое заполняется целиком.
 * @param rule  номер правила Вольфрама, 0..255.
 * @param width длина поля; приводится к диапазону 1..#CELLS_MAX_WIDTH, так
 *        что неположительное значение задаёт поле из одной клетки, а не
 *        порчу памяти на первом же шаге.
 */
void elementary_init(struct Elementary *ca, unsigned char rule, int width);

/** Начальное состояние задаётся строкой из '0' и '1'. */
bool elementary_init_from(struct Elementary *ca, unsigned char rule, const char *pattern);

/**
 * Один шаг. Края замкнуты в кольцо: клетка на левом краю соседствует с
 * клеткой на правом. Кольцо выбрано потому, что оно не вносит выделенных
 * точек — при «мёртвых» краях узор у границы вёл бы себя иначе, чем в
 * середине, и картинка врала бы о правиле.
 */
void elementary_step(struct Elementary *ca);

void elementary_print(const struct Elementary *ca, FILE *out);

/* --- игра «Жизнь» --------------------------------------------------------- */

/**
 * Сторона поля «Жизни». 256 — не требование лекции (её примеры укладываются
 * в 64), а терминальной программы life_tui: при клетке в один пиксель поле
 * равно экрану, и на широком терминале это больше двухсот клеток.
 */
#define LIFE_MAX_SIDE 256

/** Поле игры «Жизнь»; за краем поле замкнуто в тор. */
struct Life {
    int width;
    int height;
    char cells[LIFE_MAX_SIDE][LIFE_MAX_SIDE];
};

void life_init(struct Life *life, int width, int height);
void life_set(struct Life *life, int x, int y, bool alive);
bool life_get(const struct Life *life, int x, int y);

/**
 * Расстановка по имени. Левый верхний угол фигуры помещается в (x, y);
 * false — имя не распознано. Известные имена:
 *
 *   натюрморты   block, beehive, loaf, boat, tub, eater;
 *   осцилляторы  blinker, toad, beacon, pulsar, pentadecathlon;
 *   корабли      glider, lwss, mwss, hwss;
 *   эволюции     r-pentomino, diehard, acorn, cheshire-cat;
 *   рост         gosper-gun;
 *   триплеты     triplet-step, triplet-v, triplet-diagonal, tromino;
 *   без предка   garden-of-eden.
 *
 * Не из лекции, а из LifeWiki и коллекции Golly:
 *
 *   натюрморты   pond, ship, snake;
 *   осцилляторы  clock, octagon-2, figure-eight, koks-galaxy, tumbler,
 *                queen-bee-shuttle;
 *   корабли      copperhead (c/10), loafer (c/7), puffer-train;
 *   рост         simkin-gun, growth-10, growth-5x5, growth-line, quadratic-20;
 *   гептамино    b-heptomino, pi-heptomino, herschel;
 *   долгожители  rabbits, blom, iwona, justyna, lidka, natural-lwss,
 *                temp-pulsars.
 */
bool life_place(struct Life *life, const char *name, int x, int y);

/** Перебор известных имён фигур: @p index от нуля, NULL — имена кончились.
    Нужен программам, которые печатают список фигур или обходят их все. */
const char *life_pattern_name(int index);

/**
 * Размер описывающего прямоугольника фигуры — чтобы поставить её в центр
 * поля или подобрать поле, на котором она поместится.
 *
 * @return false — имя не распознано; @p width и @p height тогда не меняются.
 */
bool life_pattern_size(const char *name, int *width, int *height);

/** Один ход. Поле замкнуто в тор по обеим осям — по той же причине, что и
    кольцо у одномерного автомата. */
void life_step(struct Life *life);

/**
 * Ход по правилу чётности Фредкина: клетка жива, если число её живых соседей
 * нечётно. Соседей здесь четыре — окрестность фон Неймана, как в исходной
 * постановке; окрестность Мура даёт другое число копий.
 *
 * Правило замечательно тем, что копирует ЛЮБУЮ начальную конфигурацию: через
 * 2^n ходов она воспроизводит себя четырежды — справа, слева, сверху и
 * снизу от опустевшего места, где была. Так в книге М. Гарднера показано
 * «размножение тримино», и этим же правилом иллюстрируется тривиальное
 * самовоспроизведение (§ «Самовоспроизведение» лекции 11).
 *
 * Поле то же и структура кода та же, что у life_step: различается только
 * локальная функция переходов — и поведение меняется полностью.
 */
void life_step_parity(struct Life *life);

/**
 * Заливка поля «агаром» — регулярной решёткой из блоков 2 x 2, разделённых
 * одной пустой линией (Р. Т. Уэйнрайт). Агар устойчив: каждая живая клетка
 * имеет ровно трёх живых соседей.
 *
 * Сторона поля должна делиться на 3, иначе на стыке через тор получится шов,
 * и агар начнёт разрушаться сам по себе; функция возвращает false, если это
 * не так.
 */
bool life_fill_agar(struct Life *life);

int life_population(const struct Life *life);

/** Совпадают ли конфигурации со сдвигом на (dx, dy). Так проверяется, что
    глайдер воспроизвёл себя, а не просто выжил. */
bool life_equal_shifted(const struct Life *a, const struct Life *b, int dx, int dy);

void life_print(const struct Life *life, FILE *out);

/* --- муравей Лэнгтона ----------------------------------------------------- */

#define ANT_MAX_SIDE 128 /**< сторона поля муравья */

/** Куда смотрит муравей. */
enum AntDirection { ANT_UP, ANT_RIGHT, ANT_DOWN, ANT_LEFT };

/** Поле муравья Лэнгтона вместе с самим муравьём. */
struct Ant {
    int side;
    char cells[ANT_MAX_SIDE][ANT_MAX_SIDE];
    int x;
    int y;
    enum AntDirection dir;
    long steps;   /**< сделано шагов */
    bool escaped; /**< муравей дошёл до края: дальше поведение не моделируется */
};

void ant_init(struct Ant *ant, int side);

/**
 * Шаг муравья: на белой клетке — поворот направо, на чёрной — налево; клетка
 * перекрашивается, муравей идёт вперёд. Возвращает false, когда муравей
 * упёрся в край поля: в отличие от двух автоматов выше, замыкать поле в тор
 * здесь нельзя — «шоссе» уходит в бесконечность и на торе замкнулось бы само
 * на себя, дав поведение, которого у настоящего муравья нет.
 */
bool ant_step(struct Ant *ant);

int ant_black_count(const struct Ant *ant);

void ant_print(const struct Ant *ant, FILE *out);

#if defined(__cplusplus)
}
#endif

#endif /* STATECRAFT_CELLS_H */
