#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "post_machine.h"

struct PmMachine *pm_machine_create(void) {
    struct PmMachine *m = (struct PmMachine *)calloc(1, sizeof(struct PmMachine));

    if (m == NULL) {
        return NULL;
    }
    m->history = (struct PmUndo *)calloc(PM_HISTORY, sizeof(struct PmUndo));
    if (m->history == NULL) {
        free(m);
        return NULL;
    }
    return m;
}

void pm_machine_destroy(struct PmMachine *m) {
    if (m != NULL) {
        free(m->history);
        free(m);
    }
}

/* --- разбор --------------------------------------------------------------- */

static const char *skip_spaces(const char *c) {
    while (*c == ' ' || *c == '\t') {
        ++c;
    }
    return c;
}

/* Число без знака; -1 — в начале строки не цифра. Слишком длинное число
   ограничивается: строк в программе всё равно не больше PM_MAX_LINES. */
static int read_number(const char **c) {
    int value = 0;

    if (**c < '0' || **c > '9') {
        return -1;
    }
    while (**c >= '0' && **c <= '9') {
        if (value < 100000) {
            value = value * 10 + (**c - '0');
        }
        ++*c;
    }
    return value;
}

/* Команда в начале строки; -1 — незнакомая. Стрелки принимаются и в
   записи ASCII, как в лекции, и знаками Юникода «←», «→». */
static int read_op(const char **c) {
    static const struct {
        const char *text;
        int op;
    } ops[] = {
            {"<-", PM_LEFT},
            {"->", PM_RIGHT},
            {"\342\206\220", PM_LEFT},
            {"\342\206\222", PM_RIGHT},
            {"V", PM_MARK},
            {"v", PM_MARK},
            {"X", PM_ERASE},
            {"x", PM_ERASE},
            {"?", PM_TEST},
            {"!", PM_STOP},
    };
    size_t i;

    for (i = 0; i < sizeof(ops) / sizeof(ops[0]); ++i) {
        size_t len = strlen(ops[i].text);
        if (strncmp(*c, ops[i].text, len) == 0) {
            *c += len;
            return ops[i].op;
        }
    }
    return -1;
}

/* Комментарий без пробелов по краям; обрезается по границе символа UTF-8. */
static void copy_comment(const char *c, char *out) {
    size_t len;

    c = skip_spaces(c);
    len = strlen(c);
    while (len > 0 && (c[len - 1] == ' ' || c[len - 1] == '\t' || c[len - 1] == '\r')) {
        --len;
    }
    if (len > PM_COMMENT_SIZE - 1) {
        len = PM_COMMENT_SIZE - 1;
        while (len > 0 && ((unsigned char)c[len] & 0xC0) == 0x80) {
            --len;
        }
    }
    memcpy(out, c, len);
    out[len] = '\0';
}

int pm_parse_line(const char *text, int number, struct PmCommand *out, char *error, int size) {
    const char *c = skip_spaces(text);
    const char *save = c;
    char message[PM_ERROR_SIZE];
    int value;

    memset(out, 0, sizeof(*out));
    message[0] = '\0';
    value = read_number(&c);
    if (value >= 0) {
        if (*c != '.') {
            sprintf(message, "строка %d: после номера строки нужна точка", number);
        } else if (value != number) {
            sprintf(message, "строка %d: записан номер %d — строки нумеруются по порядку", number,
                    value);
        }
        ++c;
    } else {
        c = save;
    }
    if (message[0] == '\0') {
        c = skip_spaces(c);
        out->op = read_op(&c);
        if (out->op < 0) {
            sprintf(message, "строка %d: незнакомая команда — нужна одна из V X <- -> ? !", number);
        }
    }
    if (message[0] == '\0' && out->op != PM_STOP) {
        c = skip_spaces(c);
        out->next = read_number(&c);
        out->explicit_next = (out->next >= 0);
        if (out->op == PM_TEST) {
            c = skip_spaces(c);
            if (out->next < 0) {
                sprintf(message, "строка %d: после «?» нужны два номера строк, «? 1; 3»", number);
            } else if (*c != ';' && *c != ',') {
                sprintf(message, "строка %d: номера после «?» разделяются «;»", number);
            } else {
                c = skip_spaces(c + 1);
                out->next_marked = read_number(&c);
                if (out->next_marked < 0) {
                    sprintf(message, "строка %d: после «?» нужен второй номер строки", number);
                }
            }
        } else if (out->next < 0) {
            out->next = number + 1;
        }
    }
    if (message[0] != '\0') {
        if (error != NULL && size > 0) {
            strncpy(error, message, (size_t)size - 1);
            error[size - 1] = '\0';
        }
        return 0;
    }
    copy_comment(c, out->comment);
    return 1;
}

static void fail(struct PmMachine *m, const char *text) {
    strncpy(m->error, text, sizeof(m->error) - 1);
    m->error[sizeof(m->error) - 1] = '\0';
}

static int target_valid(struct PmMachine *m, int number, int target) {
    char text[PM_ERROR_SIZE];

    if (target >= 1 && target <= m->line_count) {
        return 1;
    }
    if (target == m->line_count + 1 && target == number + 1) {
        sprintf(text, "строка %d: за ней строк нет — последней должна стоять «!» или переход",
                number);
    } else {
        sprintf(text, "строка %d: переход к строке %d, а строк %d", number, target, m->line_count);
    }
    fail(m, text);
    return 0;
}

/* Разбор программы; 0 — ошибка, её текст в m->error. */
static int compile(struct PmMachine *m, const struct PmProgram *p) {
    int i;

    m->line_count = 0;
    if (p->lines == NULL || p->lines[0] == NULL) {
        fail(m, "в программе нет ни одной строки");
        return 0;
    }
    for (i = 0; p->lines[i] != NULL; ++i) {
        if (i >= PM_MAX_LINES) {
            fail(m, "в программе больше строк, чем помещается в машину (64)");
            return 0;
        }
        if (!pm_parse_line(p->lines[i], i + 1, &m->lines[i], m->error, (int)sizeof(m->error))) {
            return 0;
        }
    }
    m->line_count = i;
    for (i = 0; i < m->line_count; ++i) {
        const struct PmCommand *c = &m->lines[i];
        if (c->op == PM_STOP) {
            continue;
        }
        if (!target_valid(m, i + 1, c->next)
            || (c->op == PM_TEST && !target_valid(m, i + 1, c->next_marked))) {
            return 0;
        }
    }
    if (p->examples == NULL || p->examples[0] == NULL) {
        fail(m, "у программы нет ни одного примера");
        return 0;
    }
    return 1;
}

/* Проверка слова; @p cells и @p head получают длину без знака каретки и
   положение каретки от начала слова. */
static int word_valid(struct PmMachine *m, const char *word, int *cells, int *head) {
    const char *c;
    int signs = 0;
    char text[PM_ERROR_SIZE];

    *cells = 0;
    *head = 0;
    if (strlen(word) >= PM_INPUT_SIZE) {
        sprintf(text, "слово длиннее %d символов", PM_INPUT_SIZE - 1);
        fail(m, text);
        return 0;
    }
    for (c = word; *c != '\0'; ++c) {
        if (*c == PM_HEAD_SIGN) {
            if (++signs > 1) {
                fail(m, "каретка в слове отмечена дважды: знак «^» ставится один раз");
                return 0;
            }
            *head = *cells;
        } else if (*c == '0' || *c == '1') {
            ++*cells;
        } else {
            sprintf(text, "символ «%c»: слово набирается из 0 (пусто), 1 (метка) и ^ (каретка)",
                    *c);
            fail(m, text);
            return 0;
        }
    }
    return 1;
}

int pm_machine_load(struct PmMachine *m, const struct PmProgram *program) {
    int i;
    int cells;
    int head;

    m->program = NULL;
    m->error[0] = '\0';
    if (program == NULL || !compile(m, program)) {
        m->line_count = 0;
        return 0;
    }
    for (i = 0; program->examples[i] != NULL; ++i) {
        if (!word_valid(m, program->examples[i], &cells, &head)) {
            m->line_count = 0;
            return 0;
        }
    }
    m->program = program;
    return pm_machine_set_input(m, program->examples[0]);
}

void pm_machine_reset(struct PmMachine *m) {
    const char *c;
    int pos;

    memset(m->tape, 0, sizeof(m->tape));
    memset(m->written, 0, sizeof(m->written));
    m->origin = PM_TAPE_CELLS / 2;
    m->head = m->origin;
    pos = m->origin;
    for (c = m->input; *c != '\0'; ++c) {
        if (*c == PM_HEAD_SIGN) {
            m->head = pos;
        } else {
            m->tape[pos++] = (char)(*c == '1');
        }
    }
    memcpy(m->initial, m->tape, sizeof(m->tape));
    m->line = 1;
    m->step = 0;
    m->status = PM_RUNNING;
    m->last_line = 0;
    m->history_head = 0;
    m->history_count = 0;
}

int pm_machine_set_input(struct PmMachine *m, const char *word) {
    int cells;
    int head;

    if (m->program == NULL) {
        fail(m, "программа не загружена");
        return 0;
    }
    if (!word_valid(m, word, &cells, &head)) {
        return 0;
    }
    strcpy(m->input, word);
    pm_machine_reset(m);
    return 1;
}

/* --- прогон --------------------------------------------------------------- */

int pm_machine_next_line(const struct PmMachine *m) {
    if (m->program == NULL || m->status != PM_RUNNING) {
        return 0;
    }
    return m->line;
}

int pm_machine_step(struct PmMachine *m) {
    const struct PmCommand *c;
    struct PmUndo *u;
    int pos;

    if (m->program == NULL || m->status != PM_RUNNING) {
        return 0;
    }
    c = &m->lines[m->line - 1];
    pos = m->head;
    if (c->op == PM_MARK && m->tape[pos]) {
        m->status = PM_STUCK_MARK;
        return 0;
    }
    if (c->op == PM_ERASE && !m->tape[pos]) {
        m->status = PM_STUCK_EMPTY;
        return 0;
    }
    if ((c->op == PM_LEFT && pos == 0) || (c->op == PM_RIGHT && pos == PM_TAPE_CELLS - 1)) {
        /* Лента конечна; дальше пришлось бы писать мимо массива. */
        m->status = PM_TAPE_END;
        return 0;
    }

    u = &m->history[m->history_head];
    u->line = m->line;
    u->pos = pos;
    u->changed = (c->op == PM_MARK || c->op == PM_ERASE);
    u->written = m->written[pos];
    m->history_head = (m->history_head + 1) % PM_HISTORY;
    if (m->history_count < PM_HISTORY) {
        ++m->history_count;
    }

    ++m->step;
    m->last_line = m->line;
    switch (c->op) {
    case PM_MARK:
    case PM_ERASE:
        m->tape[pos] = (char)(c->op == PM_MARK);
        m->written[pos] = m->step;
        m->line = c->next;
        break;
    case PM_LEFT:
        --m->head;
        m->line = c->next;
        break;
    case PM_RIGHT:
        ++m->head;
        m->line = c->next;
        break;
    case PM_TEST:
        m->line = m->tape[pos] ? c->next_marked : c->next;
        break;
    default:
        m->status = PM_HALTED;
        break;
    }
    return 1;
}

long pm_machine_run(struct PmMachine *m, long limit) {
    long done = 0;

    while (done < limit && pm_machine_step(m)) {
        ++done;
    }
    return done;
}

/* Запись истории @p back тактов назад: 0 — последний такт. */
static const struct PmUndo *undo_at(const struct PmMachine *m, int back) {
    return &m->history[(m->history_head - 1 - back + 2 * PM_HISTORY) % PM_HISTORY];
}

int pm_machine_back(struct PmMachine *m) {
    const struct PmUndo *u;

    if (m->program == NULL || m->history_count == 0) {
        return 0;
    }
    u = undo_at(m, 0);
    if (u->changed) {
        m->tape[u->pos] = (char)!m->tape[u->pos];
    }
    m->written[u->pos] = u->written;
    m->head = u->pos;
    m->line = u->line;
    --m->step;
    m->status = PM_RUNNING;
    m->history_head = (m->history_head - 1 + PM_HISTORY) % PM_HISTORY;
    --m->history_count;
    m->last_line = (m->history_count > 0) ? undo_at(m, 0)->line : 0;
    return 1;
}

int pm_machine_view(const struct PmMachine *m, int back, int from, int count, char *cells,
                    int *head, int *line, int *changed) {
    int i;

    if (back < 0 || back > m->history_count) {
        return 0;
    }
    for (i = 0; i < count; ++i) {
        int pos = from + i;
        cells[i] = (pos >= 0 && pos < PM_TAPE_CELLS) ? m->tape[pos] : 0;
    }
    *head = m->head;
    *line = m->line;
    for (i = 0; i < back; ++i) {
        const struct PmUndo *u = undo_at(m, i);
        if (u->changed && u->pos >= from && u->pos < from + count) {
            cells[u->pos - from] = (char)!cells[u->pos - from];
        }
        *head = u->pos;
        *line = u->line;
    }
    *changed = -1;
    if (back < m->history_count) {
        const struct PmUndo *u = undo_at(m, back);
        if (u->changed) {
            *changed = u->pos;
        }
    }
    return 1;
}

int pm_machine_count(const struct PmMachine *m) {
    int n = 0;
    int i;

    for (i = 0; i < PM_TAPE_CELLS; ++i) {
        n += m->tape[i];
    }
    return n;
}

const char *pm_machine_format(const struct PmMachine *m, char *out, int size) {
    int lo = 0;
    int hi = PM_TAPE_CELLS - 1;
    int n = 0;
    int i;

    while (lo < PM_TAPE_CELLS && !m->tape[lo]) {
        ++lo;
    }
    while (hi >= 0 && !m->tape[hi]) {
        --hi;
    }
    if (lo > hi) {
        lo = hi = m->head; /* меток нет: остаётся одна каретка */
    }
    if (m->head < lo) {
        lo = m->head;
    }
    if (m->head > hi) {
        hi = m->head;
    }
    if (hi - lo + 2 > size - 1) {
        return NULL;
    }
    for (i = lo; i <= hi; ++i) {
        if (i == m->head) {
            out[n++] = PM_HEAD_SIGN;
        }
        out[n++] = (char)(m->tape[i] ? '1' : '0');
    }
    out[n] = '\0';
    return out;
}

void pm_command_text(const struct PmCommand *c, char *out) {
    static const char *const names[] = {"V", "X", "<-", "->", "?", "!"};

    if (c->op == PM_TEST) {
        sprintf(out, "? %d; %d", c->next, c->next_marked);
    } else if (c->op != PM_STOP && c->explicit_next) {
        sprintf(out, "%s %d", names[c->op], c->next);
    } else {
        strcpy(out, names[c->op]);
    }
}
