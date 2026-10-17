#include "life_session.h"

#include <stdlib.h>
#include <string.h>

static int clamp_side(int side) {
    if (side < 1) {
        return 1;
    }
    return (side > LIFE_MAX_SIDE) ? LIFE_MAX_SIDE : side;
}

static void forget_history(struct LifeSession *session) {
    session->history_head = 0;
    session->history_count = 0;
    session->generation = 0;
}

struct LifeSession *life_session_create(int width, int height) {
    struct LifeSession *session = (struct LifeSession *)malloc(sizeof(*session));

    if (session == NULL) {
        return NULL;
    }
    session->history = (struct Life *)malloc(sizeof(struct Life) * LIFE_SESSION_HISTORY);
    if (session->history == NULL) {
        free(session);
        return NULL;
    }
    life_init(&session->life, clamp_side(width), clamp_side(height));
    session->rule = LIFE_RULE_CONWAY;
    forget_history(session);
    return session;
}

void life_session_destroy(struct LifeSession *session) {
    if (session != NULL) {
        free(session->history);
        free(session);
    }
}

void life_session_resize(struct LifeSession *session, int width, int height) {
    /* Новое поле собирается во временной копии, а не на месте: при
       увеличении клетки сдвигаются вправо-вниз и затёрли бы ещё не
       перенесённые. Копия статическая — 64 КБ на стеке ни к чему. */
    static struct Life next;
    const struct Life *old = &session->life;
    int dx;
    int dy;
    int x;
    int y;

    life_init(&next, clamp_side(width), clamp_side(height));
    dx = (next.width - old->width) / 2;
    dy = (next.height - old->height) / 2;
    for (y = 0; y < old->height; ++y) {
        for (x = 0; x < old->width; ++x) {
            int nx = x + dx;
            int ny = y + dy;
            /* Без замыкания через край: срезанное при уменьшении не должно
               появиться с противоположной стороны. */
            if (old->cells[y][x] && nx >= 0 && nx < next.width && ny >= 0 && ny < next.height) {
                next.cells[ny][nx] = 1;
            }
        }
    }
    session->life = next;
    session->history_head = 0;
    session->history_count = 0;
}

void life_session_randomize(struct LifeSession *session, int percent) {
    struct Life *life = &session->life;
    int x;
    int y;

    for (y = 0; y < life->height; ++y) {
        for (x = 0; x < life->width; ++x) {
            life->cells[y][x] = (char)(rand() % 100 < percent);
        }
    }
    session->rule = LIFE_RULE_CONWAY;
    forget_history(session);
}

void life_session_forward(struct LifeSession *session) {
    session->history[session->history_head] = session->life;
    session->history_head = (session->history_head + 1) % LIFE_SESSION_HISTORY;
    if (session->history_count < LIFE_SESSION_HISTORY) {
        ++session->history_count;
    }
    if (session->rule == LIFE_RULE_PARITY) {
        life_step_parity(&session->life);
    } else {
        life_step(&session->life);
    }
    ++session->generation;
}

bool life_session_back(struct LifeSession *session) {
    if (session->history_count == 0) {
        return false;
    }
    session->history_head
            = (session->history_head + LIFE_SESSION_HISTORY - 1) % LIFE_SESSION_HISTORY;
    session->life = session->history[session->history_head];
    --session->history_count;
    --session->generation;
    return true;
}

/* --- каталог ------------------------------------------------------------- */

enum {
    SCENE_NONE,
    SCENE_EATER_GLIDER, /* пожиратель глотает планер */
    SCENE_AGAR_EDGE,    /* вирус у грани блока: агар разрушается */
    SCENE_AGAR_CORNER,  /* вирус в углу четырёх блоков: агар залечивается */
    SCENE_TRIPLETS      /* пять триплетов в ряд */
};

#define LECTURE "Лекция 11"
#define LIFEWIKI "LifeWiki"
#define GOLLY "Коллекция Golly"

/* clang-format off */
static const struct LifePreset PRESETS[] = {
    /* Объекты, разобранные в лекции 11, — в том порядке, в каком она о них
       рассказывает. */
    {"Тримино: размножение", LECTURE, "правило чётности Фредкина: через 4 хода четыре копии",
     "tromino", SCENE_NONE, LIFE_RULE_PARITY},
    {"Тримино в «Жизни»", LECTURE, "угол из трёх клеток за один ход становится блоком",
     "tromino", SCENE_NONE, LIFE_RULE_CONWAY},
    {"Пять триплетов", LECTURE, "три гибнут, тримино даёт блок, ряд — мигалку",
     NULL, SCENE_TRIPLETS, LIFE_RULE_CONWAY},
    {"Чеширский кот", LECTURE, "на 6-м ходу остаётся улыбка, на 7-м — отпечаток лапы",
     "cheshire-cat", SCENE_NONE, LIFE_RULE_CONWAY},
    {"Вирус у грани блока", LECTURE, "одна клетка разрушает агар без предела",
     NULL, SCENE_AGAR_EDGE, LIFE_RULE_CONWAY},
    {"Вирус в углу агара", LECTURE, "агар уничтожает вирус и восстанавливается за 2 хода",
     NULL, SCENE_AGAR_CORNER, LIFE_RULE_CONWAY},
    {"Сад Эдема", LECTURE, "у этой конфигурации нет предшественника (Бэнкс, 1971)",
     "garden-of-eden", SCENE_NONE, LIFE_RULE_CONWAY},
    {"Пожиратель и планер", LECTURE, "пожиратель глотает планер и восстанавливает форму",
     NULL, SCENE_EATER_GLIDER, LIFE_RULE_CONWAY},
    {"Планер", LECTURE, "c/4 по диагонали: за 4 хода клетка вправо и вниз",
     "glider", SCENE_NONE, LIFE_RULE_CONWAY},
    {"Лёгкий корабль", LECTURE, "c/2 по горизонтали", "lwss", SCENE_NONE, LIFE_RULE_CONWAY},
    {"Средний корабль", LECTURE, "c/2 по горизонтали", "mwss", SCENE_NONE, LIFE_RULE_CONWAY},
    {"Тяжёлый корабль", LECTURE, "c/2 по горизонтали", "hwss", SCENE_NONE, LIFE_RULE_CONWAY},
    {"Ружьё Госпера", LECTURE, "планер каждые 30 ходов — неограниченный рост (1970)",
     "gosper-gun", SCENE_NONE, LIFE_RULE_CONWAY},
    {"R-пентамино", LECTURE, "пять клеток и 1103 хода до стабилизации",
     "r-pentomino", SCENE_NONE, LIFE_RULE_CONWAY},
    {"Диехард", LECTURE, "семь клеток исчезают бесследно на 130-м ходу",
     "diehard", SCENE_NONE, LIFE_RULE_CONWAY},
    {"Желудь", LECTURE, "семь клеток и 5206 ходов эволюции", "acorn", SCENE_NONE,
     LIFE_RULE_CONWAY},
    {"Мигалка", LECTURE, "осциллятор периода 2", "blinker", SCENE_NONE, LIFE_RULE_CONWAY},
    {"Жаба", LECTURE, "осциллятор периода 2", "toad", SCENE_NONE, LIFE_RULE_CONWAY},
    {"Маяк", LECTURE, "осциллятор периода 2", "beacon", SCENE_NONE, LIFE_RULE_CONWAY},
    {"Пульсар", LECTURE, "осциллятор периода 3", "pulsar", SCENE_NONE, LIFE_RULE_CONWAY},
    {"Пентадекатлон", LECTURE, "ряд из десяти клеток — осциллятор периода 15",
     "pentadecathlon", SCENE_NONE, LIFE_RULE_CONWAY},
    {"Блок", LECTURE, "натюрморт", "block", SCENE_NONE, LIFE_RULE_CONWAY},
    {"Улей", LECTURE, "натюрморт", "beehive", SCENE_NONE, LIFE_RULE_CONWAY},
    {"Каравай", LECTURE, "натюрморт", "loaf", SCENE_NONE, LIFE_RULE_CONWAY},
    {"Лодка", LECTURE, "натюрморт", "boat", SCENE_NONE, LIFE_RULE_CONWAY},
    {"Кадка", LECTURE, "натюрморт", "tub", SCENE_NONE, LIFE_RULE_CONWAY},
    {"Пожиратель", LECTURE, "натюрморт, который чистит поле от планеров", "eater",
     SCENE_NONE, LIFE_RULE_CONWAY},

    /* Не из лекции: каталог LifeWiki и коллекция образцов Golly. */
    {"Ружьё Саймкина", LIFEWIKI, "планер каждые 120 ходов (М. Саймкин, 2015)",
     "simkin-gun", SCENE_NONE, LIFE_RULE_CONWAY},
    {"Медная голова", LIFEWIKI, "корабль c/10 — найден лишь в 2016 году", "copperhead",
     SCENE_NONE, LIFE_RULE_CONWAY},
    {"Бездельник", LIFEWIKI, "корабль c/7 (Дж. Болл, 2013)", "loafer", SCENE_NONE,
     LIFE_RULE_CONWAY},
    {"Пыхтящий поезд", GOLLY, "B-гептамино под конвоем двух кораблей оставляет след",
     "puffer-train", SCENE_NONE, LIFE_RULE_CONWAY},
    {"Челнок пчелиной матки", LIFEWIKI, "период 30: «пчела» мечется между блоками",
     "queen-bee-shuttle", SCENE_NONE, LIFE_RULE_CONWAY},
    {"Галактика Кока", LIFEWIKI, "осциллятор периода 8 (Я. Кок, 1971)", "koks-galaxy",
     SCENE_NONE, LIFE_RULE_CONWAY},
    {"Акробат", LIFEWIKI, "осциллятор периода 14 (tumbler)", "tumbler", SCENE_NONE,
     LIFE_RULE_CONWAY},
    {"Восьмёрка", LIFEWIKI, "осциллятор периода 8", "figure-eight", SCENE_NONE,
     LIFE_RULE_CONWAY},
    {"Восьмиугольник", LIFEWIKI, "осциллятор периода 5", "octagon-2", SCENE_NONE,
     LIFE_RULE_CONWAY},
    {"Часы", LIFEWIKI, "осциллятор периода 2", "clock", SCENE_NONE, LIFE_RULE_CONWAY},
    {"Пруд", LIFEWIKI, "натюрморт", "pond", SCENE_NONE, LIFE_RULE_CONWAY},
    {"Корабль", LIFEWIKI, "натюрморт", "ship", SCENE_NONE, LIFE_RULE_CONWAY},
    {"Змея", LIFEWIKI, "натюрморт", "snake", SCENE_NONE, LIFE_RULE_CONWAY},
    {"Десять клеток", LIFEWIKI, "десять клеток укладывают блоки бесконечно", "growth-10",
     SCENE_NONE, LIFE_RULE_CONWAY},
    {"Квадрат 5 x 5", LIFEWIKI, "бесконечный рост из квадрата 5 на 5", "growth-5x5",
     SCENE_NONE, LIFE_RULE_CONWAY},
    {"Одна линия", LIFEWIKI, "бесконечный рост из ряда толщиной в клетку", "growth-line",
     SCENE_NONE, LIFE_RULE_CONWAY},
    {"Квадратичный рост", GOLLY, "20 клеток: паровозы порождают паровозы (2022)",
     "quadratic-20", SCENE_NONE, LIFE_RULE_CONWAY},
    {"B-гептамино", LIFEWIKI, "естественный предшественник «Гершеля»", "b-heptomino",
     SCENE_NONE, LIFE_RULE_CONWAY},
    {"Пи-гептамино", LIFEWIKI, "частый гость хаоса, успокаивается за 173 хода",
     "pi-heptomino", SCENE_NONE, LIFE_RULE_CONWAY},
    {"Гершель", LIFEWIKI, "основа сигнальных схем в «Жизни»", "herschel", SCENE_NONE,
     LIFE_RULE_CONWAY},
    {"Кролики", GOLLY, "девять клеток, 17 331 ход (Э. Треворроу, 1986)", "rabbits",
     SCENE_NONE, LIFE_RULE_CONWAY},
    {"Блом", GOLLY, "13 клеток, 23 314 ходов (Д. Хикерсон, 2002)", "blom", SCENE_NONE,
     LIFE_RULE_CONWAY},
    {"Ивона", GOLLY, "19 клеток, 28 786 ходов (А. Окрасинский, 2004)", "iwona", SCENE_NONE,
     LIFE_RULE_CONWAY},
    {"Юстына", GOLLY, "20 клеток, 26 458 ходов (А. Окрасинский, 2004)", "justyna",
     SCENE_NONE, LIFE_RULE_CONWAY},
    {"Лидка", GOLLY, "13 клеток, 29 055 ходов (Д. Белл, 2005)", "lidka", SCENE_NONE,
     LIFE_RULE_CONWAY},
    {"Натуральный корабль", GOLLY, "12 клеток, лёгкий корабль на 13 811-м ходу",
     "natural-lwss", SCENE_NONE, LIFE_RULE_CONWAY},
    {"Временные пульсары", GOLLY, "дважды собирает пульсар и теряет его (1997)",
     "temp-pulsars", SCENE_NONE, LIFE_RULE_CONWAY}
};
/* clang-format on */

#define PRESET_COUNT ((int)(sizeof(PRESETS) / sizeof(PRESETS[0])))

static const char *const TRIPLETS[]
        = {"triplet-step", "triplet-v", "triplet-diagonal", "tromino", "blinker", NULL};

#define TRIPLET_GAP 5   /* пустых клеток между триплетами: не мешают друг другу */
#define PRESET_MARGIN 8 /* запас вокруг фигуры, чтобы она не задела себя через тор */
#define AGAR_SIDE 24    /* поле сцены с агаром из лекции; кратно трём */

const struct LifePreset *life_preset(int index) {
    return (index >= 0 && index < PRESET_COUNT) ? &PRESETS[index] : NULL;
}

int life_preset_count(void) {
    return PRESET_COUNT;
}

static int triplets_width(void) {
    int i;
    int total = 0;

    for (i = 0; TRIPLETS[i] != NULL; ++i) {
        int w = 0;
        int h = 0;
        (void)life_pattern_size(TRIPLETS[i], &w, &h);
        total += w + ((i > 0) ? TRIPLET_GAP : 0);
    }
    return total;
}

void life_preset_min_size(const struct LifePreset *preset, int *width, int *height) {
    int w = 0;
    int h = 0;

    switch (preset->scene) {
    case SCENE_EATER_GLIDER:
        w = 12;
        h = 12;
        break;
    case SCENE_AGAR_EDGE:
    case SCENE_AGAR_CORNER:
        /* Агар целиком — это всё поле; минимум задаёт сцена лекции. */
        *width = AGAR_SIDE;
        *height = AGAR_SIDE;
        return;
    case SCENE_TRIPLETS:
        w = triplets_width();
        h = 3;
        break;
    default:
        if (preset->pattern == NULL || !life_pattern_size(preset->pattern, &w, &h)) {
            w = 1;
            h = 1;
        }
        break;
    }
    *width = w + 2 * PRESET_MARGIN;
    *height = h + 2 * PRESET_MARGIN;
}

static bool load_agar(struct Life *life, int virus_dx) {
    int bx;
    int by;

    /* Подрезать поле до кратного трём, но не меньше одной ячейки агара. */
    life->width = (life->width < 3) ? 3 : life->width - life->width % 3;
    life->height = (life->height < 3) ? 3 : life->height - life->height % 3;
    if (!life_fill_agar(life)) {
        return false;
    }
    /* Блоки агара занимают клетки с x % 3 != 2 и y % 3 != 2. Клетка
       (bx, by + 2) лежит в пустой линии под блоком — «у грани»; клетка
       (bx + 2, by + 2) — на пересечении пустых линий, в углу четырёх
       блоков. Это те же места, что в сценах лекции. */
    bx = 3 * (life->width / 6);
    by = 3 * (life->height / 6);
    life_set(life, bx + virus_dx, by + 2, true);
    return true;
}

static bool load_triplets(struct Life *life) {
    int i;
    int x = (life->width - triplets_width()) / 2;
    int y = life->height / 2 - 1;

    for (i = 0; TRIPLETS[i] != NULL; ++i) {
        int w = 0;
        int h = 0;
        if (!life_pattern_size(TRIPLETS[i], &w, &h) || !life_place(life, TRIPLETS[i], x, y)) {
            return false;
        }
        x += w + TRIPLET_GAP;
    }
    return true;
}

static bool load_pattern(struct Life *life, const char *name) {
    int w = 0;
    int h = 0;

    if (name == NULL || !life_pattern_size(name, &w, &h)) {
        return false;
    }
    return life_place(life, name, (life->width - w) / 2, (life->height - h) / 2);
}

bool life_session_load(struct LifeSession *session, const struct LifePreset *preset) {
    struct Life *life = &session->life;
    int x;
    int y;
    bool ok;

    life_init(life, life->width, life->height);
    session->rule = preset->rule;
    forget_history(session);

    switch (preset->scene) {
    case SCENE_EATER_GLIDER:
        /* Расстановка сцены лекции «eater-vs-glider», сдвинутая в центр. */
        x = (life->width - 12) / 2;
        y = (life->height - 12) / 2;
        ok = life_place(life, "eater", x + 7, y + 7) && life_place(life, "glider", x, y);
        break;
    case SCENE_AGAR_EDGE:
        ok = load_agar(life, 0);
        break;
    case SCENE_AGAR_CORNER:
        ok = load_agar(life, 2);
        break;
    case SCENE_TRIPLETS:
        ok = load_triplets(life);
        break;
    default:
        ok = load_pattern(life, preset->pattern);
        break;
    }
    if (!ok) {
        life_init(life, life->width, life->height);
    }
    return ok;
}
