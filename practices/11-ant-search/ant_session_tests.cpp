#include <catch2/catch.hpp>
#include <ant_session.h>

#include <cstring>
#include <memory>
#include <string>

/*
 * Сеанс терминальной программы ant_tui и поиск по частям. Всё, что
 * программа показывает — такты прогона, числа в каталоге, подсветка
 * изменений, — проверяется здесь: на экран смотрит человек, а сверяет тест.
 */

namespace {

struct SessionDeleter {
    void operator()(AntSession *s) const { ant_session_destroy(s); }
};
using Session = std::unique_ptr<AntSession, SessionDeleter>;

struct SearchDeleter {
    void operator()(AntSearchState *s) const { ant_search_end(s); }
};
using Search = std::unique_ptr<AntSearchState, SearchDeleter>;

bool same_fsm(const AntFsm &a, const AntFsm &b) {
    if (a.state_count != b.state_count) {
        return false;
    }
    for (int s = 0; s < a.state_count; ++s) {
        for (int i = 0; i < 2; ++i) {
            if (a.action[s][i] != b.action[s][i] || a.next[s][i] != b.next[s][i]) {
                return false;
            }
        }
    }
    return true;
}

} /* namespace */

TEST_CASE("Каталог: каждый автомат показывает заявленный результат", "[11.AntSearch][Session]") {
    REQUIRE(ant_preset_count() >= 25);
    REQUIRE(ant_preset(ant_preset_count()) == NULL);
    REQUIRE(ant_preset(-1) == NULL);

    for (int i = 0; i < ant_preset_count(); ++i) {
        const AntPreset *p = ant_preset(i);
        INFO(p->title);
        AntFsm fsm;
        REQUIRE(ant_preset_fsm(p, &fsm));
        TrailRun run = ant_trail_run(&fsm, p->steps_limit);
        REQUIRE(run.eaten == p->eaten);
        REQUIRE(run.steps == p->steps);
        /* Число тактов из названия — то же, что даёт прогон. */
        if (p->eaten == 89) {
            REQUIRE(std::string(p->title).find(std::to_string(p->steps)) != std::string::npos);
        }
    }
}

TEST_CASE("Журнал поиска в каталоге монотонен и растёт", "[11.AntSearch][Session]") {
    int previous_steps = 315;
    int previous_states = 5;
    int records = 0;

    for (int i = 0; i < ant_preset_count(); ++i) {
        const AntPreset *p = ant_preset(i);
        if (std::strcmp(p->group, "Журнал поиска от scan") != 0) {
            continue;
        }
        INFO(p->title);
        AntFsm fsm;
        REQUIRE(ant_preset_fsm(p, &fsm));
        /* Каждая запись строго лучше предыдущей; состояния только
           добавляются — так устроен поиск с ростом при застое. */
        REQUIRE(p->steps < previous_steps);
        REQUIRE(fsm.state_count >= previous_states);
        previous_steps = p->steps;
        previous_states = fsm.state_count;
        ++records;
    }
    REQUIRE(records == 22);
    REQUIRE(previous_steps == 153);
}

TEST_CASE("Прогон по тактам совпадает с прогоном целиком", "[11.AntSearch][Session]") {
    Session s(ant_session_create());
    REQUIRE(s);
    AntFsm fsm;
    ant_fsm_evolved(&fsm);
    ant_session_show(s.get(), &fsm, TRAIL_STEPS, "evolved");

    REQUIRE_FALSE(ant_session_back(s.get()));
    int steps = 0;
    while (ant_session_forward(s.get())) {
        ++steps;
    }
    TrailRun whole = ant_trail_run(&fsm, TRAIL_STEPS);
    REQUIRE(steps == whole.steps);
    REQUIRE(ant_session_walk(s.get())->run.eaten == 89);
    REQUIRE(ant_session_walk(s.get())->run.steps == 181);
    REQUIRE(ant_session_done(s.get()));

    /* Назад — ровно к прежним тактам. */
    for (int i = 0; i < 81; ++i) {
        REQUIRE(ant_session_back(s.get()));
    }
    REQUIRE(ant_session_walk(s.get())->run.steps == 100);
    REQUIRE(ant_session_walk(s.get())->run.eaten
            == ant_trail_run(&fsm, 100).eaten);

    ant_session_to_start(s.get());
    REQUIRE(ant_session_walk(s.get())->run.steps == 0);
    ant_session_to_end(s.get());
    REQUIRE(ant_session_walk(s.get())->run.steps == 181);
}

TEST_CASE("Лимит тактов останавливает прогон того, кто не доедает", "[11.AntSearch][Session]") {
    Session s(ant_session_create());
    AntFsm fsm;
    ant_fsm_lookaround(&fsm);
    ant_session_show(s.get(), &fsm, TRAIL_STEPS, "lookaround");
    ant_session_to_end(s.get());
    REQUIRE(ant_session_walk(s.get())->run.steps == TRAIL_STEPS);
    REQUIRE(ant_session_walk(s.get())->run.eaten == 42);
    REQUIRE_FALSE(ant_session_forward(s.get()));

    ant_session_show(s.get(), &fsm, 50, "lookaround");
    ant_session_to_end(s.get());
    REQUIRE(ant_session_walk(s.get())->run.steps == 50);
}

TEST_CASE("Изменения считаются относительно показанного прежде", "[11.AntSearch][Session]") {
    Session s(ant_session_create());
    AntFsm scan;
    AntFsm next;
    ant_fsm_scan(&scan);
    REQUIRE(ant_fsm_parse(&next, "3.0.0.1:3.0.0.2:0.1.0.3:3.4.0.4:3.0.2.0"));

    ant_session_show(s.get(), &scan, TRAIL_STEPS, "scan");
    REQUIRE(s->has_previous == 0);
    ant_session_show(s.get(), &next, TRAIL_STEPS, "запись 1");
    REQUIRE(s->has_previous == 1);
    REQUIRE(same_fsm(s->previous, scan));

    /* scan:     3.0.0.1:3.0.0.2:3.0.0.3:3.0.0.4:3.0.2.0
       запись 1: 3.0.0.1:3.0.0.2:0.1.0.3:3.4.0.4:3.0.2.0 — изменились
       переход при еде у состояния 2 (действие и цель) и у состояния 3 (цель). */
    REQUIRE(ant_fsm_diff(&scan, &next, 2, 1) == (ANT_DIFF_ACTION | ANT_DIFF_NEXT));
    REQUIRE(ant_fsm_diff(&scan, &next, 3, 1) == ANT_DIFF_NEXT);
    REQUIRE(ant_fsm_diff(&scan, &next, 0, 1) == ANT_DIFF_SAME);
    REQUIRE(ant_fsm_diff_count(&scan, &next) == 2);

    AntFsm grown;
    REQUIRE(ant_fsm_parse(&grown, "3.0.0.1:3.4.0.2:1.1.0.3:3.4.0.5:3.0.2.5:3.4.2.0"));
    REQUIRE(ant_fsm_diff(&scan, &grown, 5, 0) == ANT_DIFF_NEW);
    REQUIRE(ant_fsm_diff(&scan, &grown, 5, 1) == ANT_DIFF_NEW);

    ant_session_forget_previous(s.get());
    REQUIRE(s->has_previous == 0);
}

TEST_CASE("Мнемоника перехода", "[11.AntSearch][Session]") {
    AntFsm scan;
    char text[16];
    ant_fsm_scan(&scan);
    /* scan, состояние 0: при еде — шаг и остаться (3.0), без еды — направо
       в состояние 1 (0.1). */
    ant_transition_text(&scan, 0, 1, text);
    REQUIRE(std::string(text) == "Ш0");
    ant_transition_text(&scan, 0, 0, text);
    REQUIRE(std::string(text) == "П1");
    ant_transition_text(&scan, 4, 0, text);
    REQUIRE(std::string(text) == "Ш0");
}

TEST_CASE("Случайный автомат корректен при любом размере", "[11.AntSearch][Session]") {
    AntRandom r;
    ant_random_init(&r, 7);
    const int sizes[] = {-3, 1, 5, ANT_FSM_MAX_STATES, 1000};
    for (int n : sizes) {
        AntFsm fsm;
        ant_fsm_random(&fsm, n, &r);
        REQUIRE(fsm.state_count >= 1);
        REQUIRE(fsm.state_count <= ANT_FSM_MAX_STATES);
        for (int s = 0; s < fsm.state_count; ++s) {
            for (int i = 0; i < 2; ++i) {
                REQUIRE(fsm.next[s][i] >= 0);
                REQUIRE(fsm.next[s][i] < fsm.state_count);
            }
        }
    }
}

TEST_CASE("Режимы поиска дают допустимые настройки", "[11.AntSearch][Session]") {
    AntFsm evolved;
    ant_fsm_evolved(&evolved);
    REQUIRE(ant_search_mode_count() >= 4);
    for (int i = 0; i < ant_search_mode_count(); ++i) {
        const AntSearchMode *m = ant_search_mode(i);
        INFO(m->title);
        AntSearch cfg;
        AntFsm start;
        int has_start = -1;
        ant_search_mode_config(m, 42, &evolved, &cfg, &start, &has_start);
        Search search(ant_search_begin(&cfg, has_start ? &start : NULL, NULL));
        REQUIRE(search);
        REQUIRE(cfg.seed == 42);
        if (m->start == ANT_START_CURRENT) {
            /* Автомат на экране больше потолка режима: потолок подтянут. */
            REQUIRE(same_fsm(start, evolved));
            REQUIRE(cfg.max_states >= 17);
        }
    }
}

TEST_CASE("Поиск по частям совпадает с поиском целиком", "[11.AntSearch][Session]") {
    /* Программа ищет кусками между кадрами; если бы деление на куски меняло
       ход поиска, журнал на экране разошёлся бы с журналом командной строки. */
    const AntSearchAlgorithm algorithms[] = {ANT_SEARCH_HILL, ANT_SEARCH_EP93};
    for (AntSearchAlgorithm algorithm : algorithms) {
        AntSearch cfg;
        if (algorithm == ANT_SEARCH_EP93) {
            ant_search_defaults_ep93(&cfg);
            cfg.iterations = 6000;
            cfg.population = 60;
        } else {
            ant_search_defaults(&cfg);
            cfg.iterations = 20000;
            cfg.max_states = 8;
            cfg.stall_limit = 500;
        }
        cfg.seed = 11;

        AntSearchResult whole;
        REQUIRE(ant_search_run(&cfg, NULL, &whole, NULL));

        Search search(ant_search_begin(&cfg, NULL, NULL));
        REQUIRE(search);
        int chunks = 0;
        while (!ant_search_advance(search.get(), 777, NULL)) {
            ++chunks;
        }
        const AntSearchResult *parts = ant_search_result(search.get());
        REQUIRE(chunks > 1);
        REQUIRE(parts->evaluations == whole.evaluations);
        REQUIRE(parts->improvements == whole.improvements);
        REQUIRE(parts->run.eaten == whole.run.eaten);
        REQUIRE(parts->run.steps == whole.run.steps);
        REQUIRE(same_fsm(parts->best, whole.best));
        /* Законченный поиск дальше не идёт. */
        REQUIRE(ant_search_advance(search.get(), 1000, NULL));
        REQUIRE(ant_search_result(search.get())->evaluations == whole.evaluations);
    }
}
