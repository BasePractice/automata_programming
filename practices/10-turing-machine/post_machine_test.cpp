#include <cstring>
#include <string>
#include <vector>
#include <catch2/catch.hpp>
#include <post_machine.h>

/*
  Машина Поста проверяется прогоном: у примера лекции есть названное в ней
  число тактов, у остальных программ каталога — счёт, с которым сверяется
  результат на всех коротких входах.
 */

namespace {

struct Machine {
    PmMachine *m;
    Machine() : m(pm_machine_create()) {}
    ~Machine() { pm_machine_destroy(m); }
};

const PmProgram *find(const char *title) {
    for (int i = 0; i < pm_catalog_count(); ++i) {
        if (std::strcmp(pm_catalog(i)->title, title) == 0)
            return pm_catalog(i);
    }
    FAIL("нет в каталоге: " << title);
    return nullptr;
}

int run(PmMachine *m, const PmProgram *p, const std::string &word, long limit = 1000000) {
    REQUIRE(pm_machine_load(m, p) == 1);
    REQUIRE(pm_machine_set_input(m, word.c_str()) == 1);
    pm_machine_run(m, limit);
    return m->status;
}

std::string tape(const PmMachine *m) {
    char out[PM_TAPE_CELLS + 2];
    REQUIRE(pm_machine_format(m, out, sizeof(out)) != nullptr);
    return out;
}

/* Лента без каретки: от первой метки до последней. */
std::string marks(const PmMachine *m) {
    std::string t = tape(m);
    std::string out;
    for (char c : t)
        if (c != PM_HEAD_SIGN)
            out += c;
    size_t lo = out.find('1');
    if (lo == std::string::npos)
        return "";
    return out.substr(lo, out.rfind('1') - lo + 1);
}

std::string ones(int n) { return std::string(static_cast<size_t>(n), '1'); }

/* Программа из строк теста; строки живут, пока жив объект. */
struct Text {
    std::vector<std::string> source;
    std::vector<const char *> lines;
    const char *examples[2] = {"", nullptr};
    PmProgram program{};
    explicit Text(std::vector<std::string> text) : source(std::move(text)) {
        for (const auto &s : source)
            lines.push_back(s.c_str());
        lines.push_back(nullptr);
        program.title = "тест";
        program.lines = lines.data();
        program.examples = examples;
    }
};

}  // namespace

TEST_CASE("разбор строки в записи лекции", "[post]") {
    PmCommand c;
    char error[PM_ERROR_SIZE];

    REQUIRE(pm_parse_line("2. ? 1; 3    пусто — к шагу 1, иначе — к 3", 2, &c, error,
                          sizeof(error)) == 1);
    CHECK(c.op == PM_TEST);
    CHECK(c.next == 1);
    CHECK(c.next_marked == 3);
    CHECK(std::string(c.comment) == "пусто — к шагу 1, иначе — к 3");

    REQUIRE(pm_parse_line("1. <-        шаг влево", 1, &c, error, sizeof(error)) == 1);
    CHECK(c.op == PM_LEFT);
    CHECK(c.next == 2); /* номер не записан — следующая строка */
    CHECK(c.explicit_next == 0);

    REQUIRE(pm_parse_line("-> 5", 7, &c, error, sizeof(error)) == 1);
    CHECK(c.op == PM_RIGHT);
    CHECK(c.next == 5);
    CHECK(c.explicit_next == 1);

    REQUIRE(pm_parse_line("\342\206\220", 1, &c, error, sizeof(error)) == 1); /* ← */
    CHECK(c.op == PM_LEFT);
    REQUIRE(pm_parse_line("9. !         конец", 9, &c, error, sizeof(error)) == 1);
    CHECK(c.op == PM_STOP);
    CHECK(std::string(c.comment) == "конец");

    char text[32];
    pm_parse_line("? 4;6", 5, &c, error, sizeof(error));
    pm_command_text(&c, text);
    CHECK(std::string(text) == "? 4; 6");
}

TEST_CASE("ошибки разбора", "[post]") {
    PmCommand c;
    char error[PM_ERROR_SIZE];

    CHECK(pm_parse_line("3. V", 2, &c, error, sizeof(error)) == 0); /* номер не по порядку */
    CHECK(std::strstr(error, "по порядку") != nullptr);
    CHECK(pm_parse_line("2 V", 2, &c, error, sizeof(error)) == 0); /* нет точки */
    CHECK(pm_parse_line("Y", 1, &c, error, sizeof(error)) == 0);
    CHECK(pm_parse_line("? 1", 1, &c, error, sizeof(error)) == 0);
    CHECK(pm_parse_line("? 1;", 1, &c, error, sizeof(error)) == 0);
    CHECK(pm_parse_line("?", 1, &c, error, sizeof(error)) == 0);

    Machine m;
    Text off_end({"V", "->"}); /* за последней строкой ничего нет */
    CHECK(pm_machine_load(m.m, &off_end.program) == 0);
    CHECK(std::strstr(m.m->error, "строка 2") != nullptr);
    Text far({"? 1; 7", "!"});
    CHECK(pm_machine_load(m.m, &far.program) == 0);
    CHECK(std::strstr(m.m->error, "к строке 7") != nullptr);
    Text ok({"V", "!"});
    REQUIRE(pm_machine_load(m.m, &ok.program) == 1);
    CHECK(pm_machine_set_input(m.m, "01^0") == 1);
    CHECK(pm_machine_set_input(m.m, "0^1^0") == 0);
    CHECK(pm_machine_set_input(m.m, "012") == 0);
    CHECK(std::string(m.m->input) == "01^0"); /* неудачная попытка машину не меняет */
}

TEST_CASE("каталог разбирается целиком", "[post]") {
    Machine m;
    REQUIRE(pm_catalog_count() > 0);
    for (int i = 0; i < pm_catalog_count(); ++i) {
        INFO(pm_catalog(i)->title);
        REQUIRE(pm_machine_load(m.m, pm_catalog(i)) == 1);
        for (int k = 0; pm_catalog(i)->examples[k] != nullptr; ++k)
            CHECK(pm_machine_set_input(m.m, pm_catalog(i)->examples[k]) == 1);
    }
    CHECK(pm_catalog(pm_catalog_count()) == nullptr);
}

TEST_CASE("вычитание: пример лекции", "[post]") {
    Machine m;
    const PmProgram *p = find("Вычитание P − Q");

    /* «При P = 5 и Q = 3 программа останавливается, выполнив 61 команду; на
       ленте остаются две метки». */
    REQUIRE(run(m.m, p, "00111110^111000") == PM_HALTED);
    CHECK(m.m->step == 61);
    CHECK(pm_machine_count(m.m) == 2);
    CHECK(m.m->last_line == 9);

    for (int pv = 1; pv <= 8; ++pv) {
        for (int q = 1; q <= pv; ++q) {
            INFO("P = " << pv << ", Q = " << q);
            REQUIRE(run(m.m, p, ones(pv) + "0^" + ones(q)) == PM_HALTED);
            CHECK(pm_machine_count(m.m) == pv - q);
            CHECK(marks(m.m) == ones(pv - q));
        }
    }
    /* «При Q > P она зацикливается на строках 1–2»: на конечной ленте
       практикума это край ленты. */
    CHECK(run(m.m, p, "110^1111") == PM_TAPE_END);
    CHECK((m.m->line == 1 || m.m->line == 2));
}

TEST_CASE("сложение, единица, чётность, удвоение", "[post]") {
    Machine m;

    for (int a = 1; a <= 7; ++a) {
        for (int b = 1; b <= 7; ++b) {
            INFO(a << " + " << b);
            REQUIRE(run(m.m, find("Сложение P + Q"), "^" + ones(a) + "0" + ones(b)) == PM_HALTED);
            CHECK(marks(m.m) == ones(a + b));
        }
    }
    for (int n = 1; n <= 10; ++n) {
        for (int gap = 1; gap <= 4; ++gap) {
            INFO(n << ", промежуток " << gap);
            REQUIRE(run(m.m, find("Прибавление единицы"),
                        "^" + std::string(static_cast<size_t>(gap), '0') + ones(n))
                    == PM_HALTED);
            CHECK(marks(m.m) == ones(n + 1));
        }
    }
    CHECK(run(m.m, find("Прибавление единицы"), "^111") == PM_STUCK_MARK);
    CHECK(m.m->line == 4);

    for (int n = 1; n <= 12; ++n) {
        INFO("чётность " << n);
        REQUIRE(run(m.m, find("Чётность числа"), "^" + ones(n)) == PM_HALTED);
        CHECK(pm_machine_count(m.m) == n % 2);
    }
    for (int n = 1; n <= 12; ++n) {
        INFO("удвоение " << n);
        REQUIRE(run(m.m, find("Удвоение n → 2n"), "^" + ones(n)) == PM_HALTED);
        CHECK(marks(m.m) == ones(2 * n));
    }
}

TEST_CASE("поиск метки с неизвестной стороны", "[post]") {
    Machine m;
    const PmProgram *p = find("Поиск метки");

    for (int d = -20; d <= 20; ++d) {
        if (d == 0)
            continue;
        std::string word;
        if (d > 0)
            word = "^" + std::string(static_cast<size_t>(d), '0') + "1";
        else
            word = "1" + std::string(static_cast<size_t>(-d - 1), '0') + "^0";
        INFO("метка на " << d << ": " << word);
        REQUIRE(run(m.m, p, word) == PM_HALTED);
        CHECK(pm_machine_count(m.m) == 1);            /* границы сняты */
        CHECK(m.m->tape[m.m->head] == 1);             /* каретка на метке */
        CHECK(m.m->head == m.m->origin + (d > 0 ? d : 0)); /* на той самой */
    }
}

TEST_CASE("шаг назад восстанавливает каждую конфигурацию", "[post]") {
    Machine m;
    REQUIRE(run(m.m, find("Удвоение n → 2n"), "^111", 0) == PM_RUNNING);

    std::vector<std::string> seen;
    std::vector<int> lines;
    do {
        seen.push_back(tape(m.m));
        lines.push_back(m.m->line);
    } while (pm_machine_step(m.m));
    REQUIRE(m.m->status == PM_HALTED);
    REQUIRE(m.m->step == static_cast<long>(seen.size()) - 1);

    /* Прошлое без отката: окно вокруг слова. */
    const int from = m.m->origin - 4;
    const int count = 20;
    for (long back = 0; back <= m.m->step; ++back) {
        char cells[count];
        int head, line, changed;
        REQUIRE(pm_machine_view(m.m, static_cast<int>(back), from, count, cells, &head, &line,
                                &changed) == 1);
        const size_t at = static_cast<size_t>(m.m->step - back);
        CHECK(line == lines[at]);
        (void)head;
    }
    for (long k = m.m->step; k > 0; --k) {
        REQUIRE(pm_machine_back(m.m) == 1);
        CHECK(tape(m.m) == seen[static_cast<size_t>(k - 1)]);
        CHECK(m.m->line == lines[static_cast<size_t>(k - 1)]);
        CHECK(m.m->status == PM_RUNNING);
    }
    CHECK(pm_machine_back(m.m) == 0);
    CHECK(std::memcmp(m.m->tape, m.m->initial, PM_TAPE_CELLS) == 0);
}

TEST_CASE("невыполнимая команда тактом не считается", "[post]") {
    Machine m;
    Text erase({"X", "!"});
    REQUIRE(pm_machine_load(m.m, &erase.program) == 1);
    REQUIRE(pm_machine_set_input(m.m, "^0") == 1);
    CHECK(pm_machine_step(m.m) == 0);
    CHECK(m.m->status == PM_STUCK_EMPTY);
    CHECK(m.m->step == 0);
    CHECK(pm_machine_next_line(m.m) == 0);
}
