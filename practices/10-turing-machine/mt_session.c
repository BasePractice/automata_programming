#include <stdlib.h>
#include <string.h>

#include "mt_session.h"

struct MtSession *mt_session_create(void) {
    struct MtSession *s = (struct MtSession *)calloc(1, sizeof(struct MtSession));

    if (s == NULL) {
        return NULL;
    }
    s->history = (struct MtUndo *)calloc(MT_HISTORY, sizeof(struct MtUndo));
    if (s->history == NULL) {
        free(s);
        return NULL;
    }
    s->last_rule = -1;
    return s;
}

void mt_session_destroy(struct MtSession *s) {
    if (s != NULL) {
        free(s->history);
        free(s);
    }
}

int mt_session_state_index(const struct MtSession *s, const char *name) {
    int i;

    for (i = 0; i < s->state_count; ++i) {
        if (strcmp(s->states[i], name) == 0) {
            return i;
        }
    }
    return -1;
}

/* Номер состояния; незнакомое заводится. -1 — состояний больше, чем
   MT_MAX_STATES. */
static int intern_state(struct MtSession *s, const char *name) {
    int i = mt_session_state_index(s, name);

    if (i >= 0) {
        return i;
    }
    if (s->state_count >= MT_MAX_STATES) {
        return -1;
    }
    s->states[s->state_count] = name;
    return s->state_count++;
}

static int in_alphabet(const char *alphabet, char ch) {
    return ch != '\0' && strchr(alphabet, ch) != NULL;
}

static int word_valid(const struct MtProgram *p, const char *word) {
    const char *c;

    if (strlen(word) >= MT_INPUT_SIZE) {
        return 0;
    }
    for (c = word; *c != '\0'; ++c) {
        if (!in_alphabet(p->input_alphabet, *c)) {
            return 0;
        }
    }
    return 1;
}

/* Разбор программы в таблицу; 0 — программа описана с ошибкой. */
static int compile(struct MtSession *s, const struct MtProgram *p) {
    const char *c;
    int i;

    s->state_count = 0;
    memset(s->table, 0xFF, sizeof(s->table)); /* все -1 */
    if (p->alphabet == NULL || p->alphabet[0] == '\0' || p->rule_count < 0) {
        return 0;
    }
    for (c = p->input_alphabet; *c != '\0'; ++c) {
        if (!in_alphabet(p->alphabet, *c) || *c == p->alphabet[0]) {
            return 0;
        }
    }
    /* Начальное состояние — первым, остальные — в порядке, в котором
       описаны их команды: так столбцы таблицы на экране идут как в лекции. */
    if (intern_state(s, p->start) != 0) {
        return 0;
    }
    for (i = 0; i < p->rule_count; ++i) {
        if (intern_state(s, p->rules[i].state) < 0) {
            return 0;
        }
    }
    for (i = 0; i < p->rule_count; ++i) {
        const struct MtRule *r = &p->rules[i];
        int from = intern_state(s, r->state);
        int to = intern_state(s, r->next);

        if (from < 0 || to < 0 || !in_alphabet(p->alphabet, r->read)
            || !in_alphabet(p->alphabet, r->write)
            || (r->move != 'L' && r->move != 'R' && r->move != 'S')) {
            return 0;
        }
        if (s->table[from][(unsigned char)r->read] >= 0) {
            return 0; /* машина обязана быть детерминированной */
        }
        s->table[from][(unsigned char)r->read] = (short)i;
    }
    s->halt_state = intern_state(s, p->halt);
    if (s->halt_state <= 0) {
        return 0; /* нет места или заключительное совпало с начальным */
    }
    /* Из заключительного состояния команд нет: в нём машина стоит. */
    for (i = 0; i < 256; ++i) {
        if (s->table[s->halt_state][i] >= 0) {
            return 0;
        }
    }
    if (p->examples == NULL || p->examples[0] == NULL) {
        return 0;
    }
    for (i = 0; p->examples[i] != NULL; ++i) {
        if (!word_valid(p, p->examples[i])) {
            return 0;
        }
    }
    return 1;
}

int mt_session_load(struct MtSession *s, const struct MtProgram *program) {
    s->program = NULL;
    if (program == NULL || !compile(s, program)) {
        s->state_count = 0;
        return 0;
    }
    s->program = program;
    s->input[0] = '\0';
    return mt_session_set_input(s, program->examples[0]);
}

char mt_session_blank(const struct MtSession *s) {
    return s->program->alphabet[0];
}

void mt_session_reset(struct MtSession *s) {
    int length = (int)strlen(s->input);

    memset(s->tape, mt_session_blank(s), sizeof(s->tape));
    memset(s->written, 0, sizeof(s->written));
    s->origin = MT_TAPE_CELLS / 2;
    memcpy(s->tape + s->origin, s->input, (size_t)length);
    memcpy(s->initial, s->tape, sizeof(s->tape));
    s->head = s->origin;
    if (s->program->head == MT_HEAD_LAST && length > 0) {
        s->head = s->origin + length - 1;
    }
    s->state = 0;
    s->step = 0;
    s->status = MT_RUNNING;
    s->last_rule = -1;
    s->history_head = 0;
    s->history_count = 0;
}

int mt_session_set_input(struct MtSession *s, const char *word) {
    if (s->program == NULL || !word_valid(s->program, word)) {
        return 0;
    }
    strcpy(s->input, word);
    mt_session_reset(s);
    return 1;
}

int mt_session_next_rule(const struct MtSession *s) {
    if (s->program == NULL || s->status != MT_RUNNING) {
        return -1;
    }
    return s->table[s->state][(unsigned char)s->tape[s->head]];
}

/* Вердикт распознавателя читается под головкой остановившейся машины. */
static int halted_status(const struct MtSession *s) {
    if (s->program->recognizer) {
        if (s->tape[s->head] == MT_VERDICT_YES) {
            return MT_ACCEPTED;
        }
        if (s->tape[s->head] == MT_VERDICT_NO) {
            return MT_REJECTED;
        }
    }
    return MT_HALTED;
}

int mt_session_step(struct MtSession *s) {
    const struct MtRule *r;
    struct MtUndo *u;
    int rule;
    int pos;

    if (s->program == NULL || s->status != MT_RUNNING) {
        return 0;
    }
    rule = mt_session_next_rule(s);
    if (rule < 0) {
        s->status = MT_NO_RULE;
        return 0;
    }
    r = &s->program->rules[rule];
    pos = s->head;
    if ((r->move == 'L' && pos == 0) || (r->move == 'R' && pos == MT_TAPE_CELLS - 1)) {
        /* Лента сеанса конечна; дальше пришлось бы писать мимо массива. */
        s->status = MT_TAPE_END;
        return 0;
    }

    u = &s->history[s->history_head];
    u->pos = pos;
    u->state = s->state;
    u->rule = rule;
    u->symbol = s->tape[pos];
    u->written = s->written[pos];
    u->changed = (r->write != s->tape[pos]);
    s->history_head = (s->history_head + 1) % MT_HISTORY;
    if (s->history_count < MT_HISTORY) {
        ++s->history_count;
    }

    ++s->step;
    s->tape[pos] = r->write;
    if (u->changed) {
        s->written[pos] = s->step;
    }
    if (r->move == 'L') {
        --s->head;
    } else if (r->move == 'R') {
        ++s->head;
    }
    s->state = mt_session_state_index(s, r->next);
    s->last_rule = rule;
    if (s->state == s->halt_state) {
        s->status = halted_status(s);
    }
    return 1;
}

long mt_session_run(struct MtSession *s, long limit) {
    long done = 0;

    while (done < limit && mt_session_step(s)) {
        ++done;
    }
    return done;
}

/* Запись истории @p back тактов назад: 0 — последний такт. */
static const struct MtUndo *undo_at(const struct MtSession *s, int back) {
    return &s->history[(s->history_head - 1 - back + 2 * MT_HISTORY) % MT_HISTORY];
}

int mt_session_back(struct MtSession *s) {
    const struct MtUndo *u;

    if (s->program == NULL || s->history_count == 0) {
        return 0;
    }
    u = undo_at(s, 0);
    s->tape[u->pos] = u->symbol;
    s->written[u->pos] = u->written;
    s->head = u->pos;
    s->state = u->state;
    --s->step;
    s->status = MT_RUNNING;
    s->history_head = (s->history_head - 1 + MT_HISTORY) % MT_HISTORY;
    --s->history_count;
    s->last_rule = (s->history_count > 0) ? undo_at(s, 0)->rule : -1;
    return 1;
}

int mt_session_view(const struct MtSession *s, int back, int from, int count, char *cells,
                    int *head, int *state, int *changed) {
    int i;

    if (back < 0 || back > s->history_count) {
        return 0;
    }
    for (i = 0; i < count; ++i) {
        int pos = from + i;
        cells[i] = (pos >= 0 && pos < MT_TAPE_CELLS) ? s->tape[pos] : mt_session_blank(s);
    }
    *head = s->head;
    *state = s->state;
    for (i = 0; i < back; ++i) {
        const struct MtUndo *u = undo_at(s, i);
        if (u->pos >= from && u->pos < from + count) {
            cells[u->pos - from] = u->symbol;
        }
        *head = u->pos;
        *state = u->state;
    }
    *changed = -1;
    if (back < s->history_count) {
        const struct MtUndo *u = undo_at(s, back);
        if (u->changed) {
            *changed = u->pos;
        }
    }
    return 1;
}

const char *mt_session_result(const struct MtSession *s, char *out, int size) {
    char blank = mt_session_blank(s);
    int lo = 0;
    int hi = MT_TAPE_CELLS - 1;
    int length;

    while (lo <= hi && s->tape[lo] == blank) {
        ++lo;
    }
    while (hi >= lo && s->tape[hi] == blank) {
        --hi;
    }
    length = hi - lo + 1;
    if (length + 1 > size) {
        return NULL;
    }
    memcpy(out, s->tape + lo, (size_t)length);
    out[length] = '\0';
    return out;
}

int mt_session_count(const struct MtSession *s, char symbol) {
    int n = 0;
    int i;

    for (i = 0; i < MT_TAPE_CELLS; ++i) {
        if (s->tape[i] == symbol) {
            ++n;
        }
    }
    return n;
}
