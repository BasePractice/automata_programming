#include <catch2/catch.hpp>
#include <cells.h>
#include <life_session.h>

#include <cstdlib>
#include <cstring>
#include <memory>

/*
 * Две группы проверок. Первая — фигуры, взятые не из лекции, а из LifeWiki и
 * коллекции Golly: каждая проверяется тем же, чем фигуры лекции, — периодом,
 * скоростью, ростом. Вторая — сеанс терминальной программы: всё, что
 * программа делает с полем по нажатию клавиши, проверяется здесь, а не
 * глазами.
 */

namespace {

struct SessionDeleter {
    void operator()(LifeSession *s) const { life_session_destroy(s); }
};
using Session = std::unique_ptr<LifeSession, SessionDeleter>;

/* Поле на стеке занимает 64 КБ; в тестах с несколькими полями — в куче. */
std::unique_ptr<Life> make_life(int side) {
    std::unique_ptr<Life> life(new Life);
    life_init(life.get(), side, side);
    return life;
}

/* Наименьший период фигуры, поставленной на поле side x side, не больше limit;
   0 — фигура не повторилась. */
int period_of(const char *name, int side, int limit) {
    auto start = make_life(side);
    REQUIRE(life_place(start.get(), name, side / 3, side / 3));
    auto a = make_life(side);
    *a = *start;
    for (int i = 1; i <= limit; ++i) {
        life_step(a.get());
        if (life_equal_shifted(a.get(), start.get(), 0, 0)) {
            return i;
        }
    }
    return 0;
}

const LifePreset *preset_titled(const char *title) {
    for (int i = 0; life_preset(i) != NULL; ++i) {
        if (std::strcmp(life_preset(i)->title, title) == 0) {
            return life_preset(i);
        }
    }
    return NULL;
}

} /* namespace */

/* --- фигуры из LifeWiki и Golly ------------------------------------------ */

TEST_CASE("Натюрморты из LifeWiki не меняются", "[11.Cells][LifeWiki]") {
    const char *names[] = {"pond", "ship", "snake"};
    for (const char *name : names) {
        INFO(name);
        REQUIRE(period_of(name, 16, 1) == 1);
    }
}

TEST_CASE("Осцилляторы из LifeWiki имеют заявленный период", "[11.Cells][LifeWiki]") {
    struct Case {
        const char *name;
        int period;
    };
    const Case cases[] = {{"clock", 2},        {"octagon-2", 5}, {"figure-eight", 8},
                          {"koks-galaxy", 8}, {"tumbler", 14},  {"queen-bee-shuttle", 30}};
    for (const Case &c : cases) {
        INFO(c.name);
        /* Наименьший период, а не просто повтор: фигура периода 2 повторяется
           и через 8 ходов, и такой тест не отличил бы «восьмёрку» от мигалки. */
        REQUIRE(period_of(c.name, 48, c.period) == c.period);
    }
}

TEST_CASE("Медленные корабли: медная голова c/10, бездельник c/7", "[11.Cells][LifeWiki]") {
    struct Case {
        const char *name;
        int period;
        int dx;
        int dy;
    };
    /* life_equal_shifted(a, start, dx, dy) сравнивает a(x, y) со start(x + dx,
       y + dy): медная голова за период поднимается на клетку, бездельник
       сдвигается на клетку влево. */
    const Case cases[] = {{"copperhead", 10, 0, 1}, {"loafer", 7, 1, 0}};
    for (const Case &c : cases) {
        INFO(c.name);
        auto start = make_life(40);
        REQUIRE(life_place(start.get(), c.name, 15, 15));
        auto a = make_life(40);
        *a = *start;
        for (int i = 1; i <= c.period; ++i) {
            life_step(a.get());
            if (i < c.period) {
                REQUIRE_FALSE(life_equal_shifted(a.get(), start.get(), c.dx, c.dy));
            }
        }
        REQUIRE(life_equal_shifted(a.get(), start.get(), c.dx, c.dy));
    }
}

TEST_CASE("Ружьё Саймкина выпускает планер раз в 120 ходов", "[11.Cells][LifeWiki]") {
    auto a = make_life(200);
    REQUIRE(life_place(a.get(), "simkin-gun", 80, 80));
    REQUIRE(life_population(a.get()) == 36);
    for (int period = 1; period <= 2; ++period) {
        for (int i = 0; i < 120; ++i) {
            life_step(a.get());
        }
        REQUIRE(life_population(a.get()) == 36 + 5 * period);
    }
}

TEST_CASE("Фигуры неограниченного роста растут", "[11.Cells][LifeWiki]") {
    const char *names[] = {"growth-10", "growth-5x5", "growth-line", "puffer-train"};
    for (const char *name : names) {
        INFO(name);
        auto a = make_life(LIFE_MAX_SIDE);
        REQUIRE(life_place(a.get(), name, 100, 100));
        int start = life_population(a.get());
        for (int i = 0; i < 1000; ++i) {
            life_step(a.get());
        }
        /* Число клеток не растёт монотонно — обломки сгорают, — но за
           тысячу ходов оно обязано вырасти многократно: так ведёт себя
           только фигура, которая укладывает за собой мусор без конца. */
        REQUIRE(life_population(a.get()) > 5 * start);
    }
}

TEST_CASE("Число клеток фигур совпадает с источником", "[11.Cells][LifeWiki]") {
    /* Числа — из описаний коллекции Golly и LifeWiki. Записанная с ошибкой
       фигура почти всегда теряет или приобретает клетку. */
    struct Case {
        const char *name;
        int cells;
    };
    const Case cases[] = {{"rabbits", 9},     {"blom", 13},          {"iwona", 19},
                          {"justyna", 20},    {"lidka", 13},         {"natural-lwss", 12},
                          {"temp-pulsars", 32}, {"quadratic-20", 20}, {"b-heptomino", 7},
                          {"pi-heptomino", 7}, {"herschel", 7},      {"growth-10", 10},
                          {"growth-5x5", 13}, {"growth-line", 28},   {"copperhead", 28}};
    for (const Case &c : cases) {
        INFO(c.name);
        auto a = make_life(LIFE_MAX_SIDE);
        REQUIRE(life_place(a.get(), c.name, 10, 10));
        REQUIRE(life_population(a.get()) == c.cells);
    }
}

TEST_CASE("Размер фигуры по имени", "[11.Cells]") {
    int w = -1;
    int h = -1;
    REQUIRE(life_pattern_size("garden-of-eden", &w, &h));
    REQUIRE(w == 33);
    REQUIRE(h == 9);
    REQUIRE(life_pattern_size("growth-line", &w, &h));
    REQUIRE(w == 39);
    REQUIRE(h == 1);
    REQUIRE_FALSE(life_pattern_size("нет такой", &w, &h));
    REQUIRE(w == 39);
}

/* --- сеанс ----------------------------------------------------------------- */

TEST_CASE("Шаг назад возвращает ровно то, что было", "[11.Cells][Session]") {
    Session s(life_session_create(40, 30));
    REQUIRE(s);
    REQUIRE(life_session_load(s.get(), preset_titled("Планер")));
    auto start = make_life(1);
    *start = s->life;

    REQUIRE_FALSE(life_session_back(s.get()));
    for (int i = 0; i < 10; ++i) {
        life_session_forward(s.get());
    }
    REQUIRE(s->generation == 10);
    REQUIRE_FALSE(life_equal_shifted(&s->life, start.get(), 0, 0));
    for (int i = 0; i < 10; ++i) {
        REQUIRE(life_session_back(s.get()));
    }
    REQUIRE(s->generation == 0);
    REQUIRE(life_equal_shifted(&s->life, start.get(), 0, 0));
    REQUIRE_FALSE(life_session_back(s.get()));
}

TEST_CASE("История помнит не больше заданного числа поколений", "[11.Cells][Session]") {
    Session s(life_session_create(30, 30));
    life_session_randomize(s.get(), 30);
    for (int i = 0; i < LIFE_SESSION_HISTORY + 50; ++i) {
        life_session_forward(s.get());
    }
    int back = 0;
    while (life_session_back(s.get())) {
        ++back;
    }
    REQUIRE(back == LIFE_SESSION_HISTORY);
    REQUIRE(s->generation == 50);

    /* Ход вперёд после возврата — снова тот же автомат: поколение 51
       вычисляется заново и совпадает с тем, что было. */
    life_session_forward(s.get());
    REQUIRE(s->generation == 51);
}

TEST_CASE("Изменение размера держит содержимое по центру", "[11.Cells][Session]") {
    Session s(life_session_create(10, 10));
    life_set(&s->life, 4, 4, true);
    life_set(&s->life, 5, 5, true);

    life_session_resize(s.get(), 20, 14);
    REQUIRE(s->life.width == 20);
    REQUIRE(s->life.height == 14);
    REQUIRE(life_population(&s->life) == 2);
    REQUIRE(life_get(&s->life, 9, 6));
    REQUIRE(life_get(&s->life, 10, 7));

    /* Уменьшение срезает края и не переносит срезанное через тор. */
    life_set(&s->life, 0, 0, true);
    life_session_resize(s.get(), 4, 4);
    REQUIRE(life_population(&s->life) == 2);
    REQUIRE(life_get(&s->life, 1, 1));
    REQUIRE(life_get(&s->life, 2, 2));

    life_session_resize(s.get(), 100000, -3);
    REQUIRE(s->life.width == LIFE_MAX_SIDE);
    REQUIRE(s->life.height == 1);
}

TEST_CASE("Случайная расстановка задаёт плотность и правило Конуэя", "[11.Cells][Session]") {
    Session s(life_session_create(50, 40));
    s->rule = LIFE_RULE_PARITY;
    std::srand(1);
    life_session_randomize(s.get(), 0);
    REQUIRE(life_population(&s->life) == 0);
    REQUIRE(s->rule == LIFE_RULE_CONWAY);
    life_session_randomize(s.get(), 100);
    REQUIRE(life_population(&s->life) == 50 * 40);
    life_session_randomize(s.get(), 30);
    int n = life_population(&s->life);
    REQUIRE(n > 400);
    REQUIRE(n < 800);
}

TEST_CASE("Каталог: каждый объект ставится на поле своего размера", "[11.Cells][Session]") {
    Session s(life_session_create(10, 10));
    int count = life_preset_count();
    REQUIRE(count >= 50);
    REQUIRE(life_preset(count) == NULL);
    REQUIRE(life_preset(-1) == NULL);

    for (int i = 0; i < count; ++i) {
        const LifePreset *p = life_preset(i);
        INFO(p->title);
        REQUIRE(std::strlen(p->title) > 0);
        REQUIRE(std::strlen(p->group) > 0);
        REQUIRE(std::strlen(p->note) > 0);
        int w = 0;
        int h = 0;
        life_preset_min_size(p, &w, &h);
        REQUIRE(w <= LIFE_MAX_SIDE);
        REQUIRE(h <= LIFE_MAX_SIDE);
        life_session_resize(s.get(), w, h);
        life_session_forward(s.get());
        REQUIRE(life_session_load(s.get(), p));
        REQUIRE(s->generation == 0);
        REQUIRE_FALSE(life_session_back(s.get()));
        REQUIRE(life_population(&s->life) > 0);
        REQUIRE(s->rule == p->rule);
    }
}

TEST_CASE("Объекты лекции, о которых просили, в каталоге есть", "[11.Cells][Session]") {
    const char *titles[] = {"Тримино: размножение", "Чеширский кот", "Вирус у грани блока",
                            "Вирус в углу агара", "Сад Эдема"};
    for (const char *title : titles) {
        INFO(title);
        REQUIRE(preset_titled(title) != NULL);
    }
}

TEST_CASE("Тримино из каталога размножается по правилу чётности", "[11.Cells][Session]") {
    Session s(life_session_create(60, 60));
    REQUIRE(life_session_load(s.get(), preset_titled("Тримино: размножение")));
    REQUIRE(s->rule == LIFE_RULE_PARITY);
    REQUIRE(life_population(&s->life) == 3);
    for (int i = 0; i < 4; ++i) {
        life_session_forward(s.get());
    }
    /* Четыре копии по три клетки — как на рисунке лекции. */
    REQUIRE(life_population(&s->life) == 12);
}

TEST_CASE("Вирус в агаре из каталога: угол залечивается, грань — нет", "[11.Cells][Session]") {
    Session s(life_session_create(50, 41));
    auto agar = make_life(1);

    REQUIRE(life_session_load(s.get(), preset_titled("Вирус в углу агара")));
    /* Поле подрезано до кратного трём, иначе агар распался бы на шве. */
    REQUIRE(s->life.width == 48);
    REQUIRE(s->life.height == 39);
    life_init(agar.get(), 48, 39);
    REQUIRE(life_fill_agar(agar.get()));
    REQUIRE(life_population(&s->life) == life_population(agar.get()) + 1);
    life_session_forward(s.get());
    life_session_forward(s.get());
    REQUIRE(life_equal_shifted(&s->life, agar.get(), 0, 0));

    life_session_resize(s.get(), 50, 41);
    REQUIRE(life_session_load(s.get(), preset_titled("Вирус у грани блока")));
    for (int i = 0; i < 20; ++i) {
        life_session_forward(s.get());
    }
    REQUIRE_FALSE(life_equal_shifted(&s->life, agar.get(), 0, 0));
}

TEST_CASE("Пять триплетов из каталога: остаются блок и мигалка", "[11.Cells][Session]") {
    Session s(life_session_create(60, 20));
    REQUIRE(life_session_load(s.get(), preset_titled("Пять триплетов")));
    REQUIRE(life_population(&s->life) == 15);
    for (int i = 0; i < 4; ++i) {
        life_session_forward(s.get());
    }
    REQUIRE(life_population(&s->life) == 4 + 3);
}

TEST_CASE("Пожиратель из каталога съедает планер", "[11.Cells][Session]") {
    Session s(life_session_create(30, 30));
    REQUIRE(life_session_load(s.get(), preset_titled("Пожиратель и планер")));
    REQUIRE(life_population(&s->life) == 7 + 5);
    for (int i = 0; i < 40; ++i) {
        life_session_forward(s.get());
    }
    REQUIRE(life_population(&s->life) == 7);
}
