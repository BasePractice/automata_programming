#include <cstring>
#include <string>
#include <vector>
#include <catch2/catch.hpp>
#include <mt_programs.h>
#include <mt_session.h>

/*
  Каталог терминальной программы проверяется прогоном, а не чтением: у каждой
  машины есть независимый ответ — счёт на всех коротких словах, известные
  числа задачи об усердном бобре или вторая реализация той же таблицы
  (mt_programs.c) на учебном интерпретаторе.
 */

namespace {

struct Session {
    MtSession *s;
    Session() : s(mt_session_create()) {}
    ~Session() { mt_session_destroy(s); }
};

const MtProgram *find(const char *title) {
    for (int i = 0; i < mt_catalog_count(); ++i) {
        if (std::strcmp(mt_catalog(i)->title, title) == 0)
            return mt_catalog(i);
    }
    FAIL("нет в каталоге: " << title);
    return nullptr;
}

/* Прогон до остановки; лимит защищает тест от незавершающейся машины. */
int run(MtSession *s, const MtProgram *p, const std::string &word, long limit = 1000000) {
    REQUIRE(mt_session_load(s, p) == 1);
    REQUIRE(mt_session_set_input(s, word.c_str()) == 1);
    mt_session_run(s, limit);
    return s->status;
}

std::string result(const MtSession *s) {
    char out[MT_TAPE_CELLS + 1];
    REQUIRE(mt_session_result(s, out, sizeof(out)) != nullptr);
    return out;
}

/* Все слова над алфавитом длиной до max_len, включая пустое. */
std::vector<std::string> words(const std::string &alphabet, size_t max_len) {
    std::vector<std::string> all(1, "");
    size_t begin = 0;
    for (size_t len = 1; len <= max_len; ++len) {
        size_t end = all.size();
        for (size_t i = begin; i < end; ++i)
            for (char c : alphabet)
                all.push_back(all[i] + c);
        begin = end;
    }
    return all;
}

int verdict(int status) {
    if (status == MT_ACCEPTED)
        return 1;
    if (status == MT_REJECTED)
        return 0;
    return -1;
}

std::string binary(long n) {
    if (n == 0)
        return "0";
    std::string out;
    for (; n > 0; n /= 2)
        out.insert(out.begin(), char('0' + n % 2));
    return out;
}

} // namespace

TEST_CASE("Каталог разбирается целиком", "[10.Turing_Machine]") {
    Session t;
    REQUIRE(mt_catalog_count() >= 20);
    REQUIRE(mt_catalog(mt_catalog_count()) == nullptr);
    for (int i = 0; i < mt_catalog_count(); ++i) {
        const MtProgram *p = mt_catalog(i);
        INFO(p->title);
        REQUIRE(mt_session_load(t.s, p) == 1);
        REQUIRE(t.s->states[0] == std::string(p->start));
        for (int k = 0; p->examples[k] != nullptr; ++k)
            REQUIRE(mt_session_set_input(t.s, p->examples[k]) == 1);
    }
}

TEST_CASE("Ошибки описания программы не проходят разбор", "[10.Turing_Machine]") {
    Session t;
    static const char *const ex[] = {"a", nullptr};
    static const MtRule twice[] = {{"q1", 'a', 'a', 'R', "q0"}, {"q1", 'a', 'b', 'L', "q0"}};
    static const MtRule stranger[] = {{"q1", 'z', 'a', 'R', "q0"}};
    static const MtRule move[] = {{"q1", 'a', 'a', 'X', "q0"}};
    static const MtRule from_halt[] = {{"q0", 'a', 'a', 'R', "q1"}};
    MtProgram p = {"t", "t", "", "", "", "_ab", "ab", "q1", "q0", MT_HEAD_FIRST, 0, ex, twice, 2};

    REQUIRE(mt_session_load(t.s, &p) == 0);
    p.rules = stranger;
    p.rule_count = 1;
    REQUIRE(mt_session_load(t.s, &p) == 0);
    p.rules = move;
    REQUIRE(mt_session_load(t.s, &p) == 0);
    p.rules = from_halt;
    REQUIRE(mt_session_load(t.s, &p) == 0);
    REQUIRE(t.s->program == nullptr);
}

TEST_CASE("Входное слово вне алфавита не ставится", "[10.Turing_Machine]") {
    Session t;
    REQUIRE(mt_session_load(t.s, find("Палиндромы {a, b}")) == 1);
    REQUIRE(mt_session_set_input(t.s, "abc") == 0);
    REQUIRE(mt_session_set_input(t.s, "_") == 0); /* пустой символ — не буква слова */
    REQUIRE(std::string(t.s->input) == "aba");
    REQUIRE(mt_session_set_input(t.s, std::string(MT_INPUT_SIZE, 'a').c_str()) == 0);
}

TEST_CASE("Машины лекции совпадают с mt_programs.c", "[10.Turing_Machine]") {
    Session t;
    for (const std::string &w : words("ab", 7)) {
        INFO(w);
        REQUIRE(verdict(run(t.s, find("Язык aⁿbⁿ"), w)) == mt_recognize(mt_anbn_create, w.c_str()));
        REQUIRE(verdict(run(t.s, find("Палиндромы {a, b}"), w))
                == mt_recognize(mt_palindrome_create, w.c_str()));
    }
    for (const std::string &w : words("ab", 4)) {
        char out[64];
        INFO(w);
        REQUIRE(run(t.s, find("Копирование w → ww"), w) == MT_HALTED);
        REQUIRE(mt_transform(mt_copy_create, w.c_str(), out, sizeof(out)) != nullptr);
        REQUIRE(result(t.s) == out);
        REQUIRE(result(t.s) == w + w);
    }
}

TEST_CASE("M1, M2 и M3 лекции", "[10.Turing_Machine]") {
    Session t;
    REQUIRE(run(t.s, find("M1: шаг вправо и стоп"), "111") == MT_HALTED);
    REQUIRE(t.s->step == 1);
    REQUIRE(t.s->head == t.s->origin + 1);

    REQUIRE(run(t.s, find("M2: дописать символ слева"), "111") == MT_HALTED);
    REQUIRE(result(t.s) == "1111");
    REQUIRE(t.s->head == t.s->origin - 1);

    REQUIRE(run(t.s, find("M3: не останавливается"), "111", 10000) == MT_RUNNING);
    REQUIRE(t.s->step == 10000);
    REQUIRE(result(t.s) == "111");
}

TEST_CASE("Десятичные y = x + 1 и y = 2x", "[10.Turing_Machine]") {
    Session t;
    for (long x = 0; x < 2000; ++x) {
        INFO(x);
        REQUIRE(run(t.s, find("y = x + 1 (десятичная)"), std::to_string(x)) == MT_HALTED);
        REQUIRE(result(t.s) == std::to_string(x + 1));
        REQUIRE(run(t.s, find("y = 2x (десятичная)"), std::to_string(x)) == MT_HALTED);
        REQUIRE(result(t.s) == std::to_string(2 * x));
    }
}

TEST_CASE("Языки aⁿbⁿcⁿ, скобки и {w#w}", "[10.Turing_Machine]") {
    Session t;
    for (const std::string &w : words("abc", 7)) {
        size_t n = w.size() / 3;
        bool in = n > 0 && w == std::string(n, 'a') + std::string(n, 'b') + std::string(n, 'c');
        INFO(w);
        REQUIRE(verdict(run(t.s, find("Язык aⁿbⁿcⁿ"), w)) == (in ? 1 : 0));
    }
    for (const std::string &w : words("()", 10)) {
        int depth = 0;
        bool ok = true;
        for (char c : w) {
            depth += (c == '(') ? 1 : -1;
            ok = ok && depth >= 0;
        }
        INFO(w);
        REQUIRE(verdict(run(t.s, find("Баланс скобок"), w)) == ((ok && depth == 0) ? 1 : 0));
    }
    for (const std::string &w : words("01#", 7)) {
        size_t hash = w.find('#');
        bool in = hash != std::string::npos && w.find('#', hash + 1) == std::string::npos
                  && w.substr(0, hash) == w.substr(hash + 1);
        INFO(w);
        REQUIRE(verdict(run(t.s, find("Язык {w#w}"), w)) == (in ? 1 : 0));
    }
}

TEST_CASE("Двоичная и унарная арифметика", "[10.Turing_Machine]") {
    Session t;
    for (long x = 0; x < 1024; ++x) {
        INFO(x);
        REQUIRE(run(t.s, find("Двоичный инкремент"), binary(x)) == MT_HALTED);
        REQUIRE(result(t.s) == binary(x + 1));
    }
    for (int n = 0; n <= 40; ++n) {
        INFO(n);
        REQUIRE(run(t.s, find("Унарная → двоичная"), std::string(n, '1')) == MT_HALTED);
        REQUIRE(result(t.s) == binary(n));
    }
    for (int m = 0; m <= 6; ++m) {
        for (int n = 0; n <= 6; ++n) {
            std::string a(m, '1'), b(n, '1');
            INFO(m << " и " << n);
            REQUIRE(run(t.s, find("Сложение 1ᵐ + 1ⁿ"), a + "+" + b) == MT_HALTED);
            REQUIRE(result(t.s) == std::string(m + n, '1'));
            REQUIRE(run(t.s, find("Умножение 1ᵐ * 1ⁿ"), a + "*" + b + "=") == MT_HALTED);
            REQUIRE(result(t.s) == a + "*" + b + "=" + std::string(m * n, '1'));
        }
    }
}

TEST_CASE("Двоичный счётчик перебирает числа по порядку", "[10.Turing_Machine]") {
    Session t;
    REQUIRE(mt_session_load(t.s, find("Двоичный счётчик")) == 1);
    int carry = mt_session_state_index(t.s, "q1");
    long expected = 0;
    /* Каждый раз, когда машина вернулась к правому краю и начала перенос, на
       ленте стоит очередное число. */
    while (expected < 300) {
        int before = t.s->state;
        REQUIRE(mt_session_step(t.s) == 1);
        if (before != carry && t.s->state == carry) {
            ++expected;
            REQUIRE(result(t.s) == binary(expected));
        }
    }
    REQUIRE(t.s->status == MT_RUNNING);
}

TEST_CASE("Усердные бобры дают известные числа", "[10.Turing_Machine]") {
    Session t;
    struct {
        const char *title;
        long steps;
        int ones;
    } known[] = {{"Бобёр, 2 состояния", 6, 4},
                 {"Бобёр, 3 состояния: такты", 21, 5},
                 {"Бобёр, 3 состояния: единицы", 14, 6},
                 {"Бобёр, 4 состояния", 107, 13}};
    for (const auto &k : known) {
        INFO(k.title);
        REQUIRE(run(t.s, find(k.title), "") == MT_HALTED);
        REQUIRE(t.s->step == k.steps);
        REQUIRE(mt_session_count(t.s, '1') == k.ones);
    }
}

TEST_CASE("Шаг назад возвращает ровно прежнюю конфигурацию", "[10.Turing_Machine]") {
    Session t;
    REQUIRE(mt_session_load(t.s, find("Умножение 1ᵐ * 1ⁿ")) == 1);
    std::vector<std::string> tapes;
    std::vector<int> heads, states;
    do {
        tapes.push_back(std::string(t.s->tape, MT_TAPE_CELLS));
        heads.push_back(t.s->head);
        states.push_back(t.s->state);
    } while (mt_session_step(t.s));
    REQUIRE(t.s->status == MT_HALTED);
    long last = t.s->step;
    REQUIRE(last + 1 == (long)tapes.size());

    SECTION("откат") {
        for (long k = last; k > 0; --k) {
            REQUIRE(mt_session_back(t.s) == 1);
            REQUIRE(t.s->step == k - 1);
            REQUIRE(std::string(t.s->tape, MT_TAPE_CELLS) == tapes[k - 1]);
            REQUIRE(t.s->head == heads[k - 1]);
            REQUIRE(t.s->state == states[k - 1]);
            REQUIRE(t.s->status == MT_RUNNING);
        }
        REQUIRE(mt_session_back(t.s) == 0);
        REQUIRE(t.s->last_rule == -1);
        REQUIRE(std::memcmp(t.s->written, std::vector<long>(MT_TAPE_CELLS, 0).data(),
                            sizeof(t.s->written)) == 0);
    }

    SECTION("прошлое без отката") {
        std::vector<char> cells(MT_TAPE_CELLS);
        for (long back = 0; back <= last; ++back) {
            int head, state, changed;
            REQUIRE(mt_session_view(t.s, (int)back, 0, MT_TAPE_CELLS, cells.data(), &head, &state,
                                    &changed)
                    == 1);
            long k = last - back;
            REQUIRE(std::string(cells.begin(), cells.end()) == tapes[k]);
            REQUIRE(head == heads[k]);
            REQUIRE(state == states[k]);
            if (k > 0 && changed >= 0)
                REQUIRE(tapes[k][changed] != tapes[k - 1][changed]);
            if (k > 0 && changed < 0)
                REQUIRE(tapes[k] == tapes[k - 1]);
        }
        int head, state, changed;
        REQUIRE(mt_session_view(t.s, (int)last + 1, 0, 1, cells.data(), &head, &state, &changed)
                == 0);
    }
}

TEST_CASE("Отметки изменённых ячеек", "[10.Turing_Machine]") {
    Session t;
    REQUIRE(mt_session_load(t.s, find("y = x + 1 (десятичная)")) == 1);
    REQUIRE(mt_session_set_input(t.s, "199") == 1);
    int o = t.s->origin;
    REQUIRE(mt_session_step(t.s) == 1); /* 9 -> 0, влево */
    REQUIRE(t.s->written[o + 2] == 1);
    REQUIRE(t.s->tape[o + 2] != t.s->initial[o + 2]);
    REQUIRE(mt_session_next_rule(t.s) >= 0);
    REQUIRE(t.s->program->rules[mt_session_next_rule(t.s)].read == '9');
    mt_session_run(t.s, 100);
    REQUIRE(t.s->status == MT_HALTED);
    REQUIRE(t.s->step == 3);
    REQUIRE(t.s->written[o] == 3);
    REQUIRE(t.s->written[o + 1] == 2);
    REQUIRE(mt_session_next_rule(t.s) == -1);
}

TEST_CASE("Край ленты и отсутствие команды", "[10.Turing_Machine]") {
    Session t;
    /* M1 на пустом слове не встречает a1 и бежит вправо до края ленты. */
    REQUIRE(run(t.s, find("M1: шаг вправо и стоп"), "") == MT_TAPE_END);
    REQUIRE(t.s->head == MT_TAPE_CELLS - 1);
    REQUIRE(mt_session_step(t.s) == 0);
    REQUIRE(mt_session_back(t.s) == 1);
    REQUIRE(t.s->status == MT_RUNNING);

    /* Сложение без «+»: на пустой ячейке справа команды нет. */
    REQUIRE(run(t.s, find("Сложение 1ᵐ + 1ⁿ"), "111") == MT_NO_RULE);
    REQUIRE(t.s->step == 3);
}
