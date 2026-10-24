/*
 * Лекция 10. Машина Поста в командной строке: прогон программы из каталога
 * или из файла на входном слове, итог и, по ключу --trace, протокол такт за
 * тактом.
 *
 *   10-post-machine --list
 *   10-post-machine --program 1 --trace
 *   10-post-machine --file prog.txt --input "^000111"
 *
 * Файл программы — строки в записи лекции («2. ? 1; 3   комментарий»);
 * пустые строки и строки, начинающиеся с #, пропускаются. Лента печатается в
 * записи входного слова: 0 — пусто, 1 — метка, ^ — перед кареткой.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "post_machine.h"

#define LINE_SIZE 256
#define DEFAULT_LIMIT 100000L

static const char *status_text(int status) {
    switch (status) {
    case PM_HALTED:
        return "остановилась";
    case PM_STUCK_MARK:
        return "невыполнимая команда: метка в помеченную ячейку";
    case PM_STUCK_EMPTY:
        return "невыполнимая команда: стирание в пустой ячейке";
    case PM_TAPE_END:
        return "каретка упёрлась в край ленты";
    default:
        return "работает";
    }
}

/* Программа из файла: строки живут в статическом буфере до конца прогона. */
static char file_text[PM_MAX_LINES][LINE_SIZE];
static const char *file_lines[PM_MAX_LINES + 1];
static const char *file_examples[2];

static int read_program(const char *path, const char *input, struct PmProgram *p) {
    FILE *f = fopen(path, "r");
    char buffer[LINE_SIZE];
    int n = 0;

    if (f == NULL) {
        fprintf(stderr, "post: не открывается файл «%s»\n", path);
        return 0;
    }
    while (fgets(buffer, (int)sizeof(buffer), f) != NULL) {
        size_t len = strlen(buffer);
        const char *c = buffer;

        while (len > 0 && (buffer[len - 1] == '\n' || buffer[len - 1] == '\r')) {
            buffer[--len] = '\0';
        }
        while (*c == ' ' || *c == '\t') {
            ++c;
        }
        if (*c == '\0' || *c == '#') {
            continue;
        }
        if (n >= PM_MAX_LINES) {
            fprintf(stderr, "post: в файле больше %d строк программы\n", PM_MAX_LINES);
            fclose(f);
            return 0;
        }
        strcpy(file_text[n], buffer);
        file_lines[n] = file_text[n];
        ++n;
    }
    fclose(f);
    file_lines[n] = NULL;
    file_examples[0] = (input != NULL) ? input : "";
    file_examples[1] = NULL;
    memset(p, 0, sizeof(*p));
    p->title = path;
    p->group = "файл";
    p->note = "";
    p->source = path;
    p->lines = file_lines;
    p->examples = file_examples;
    return 1;
}

static void print_tape(const struct PmMachine *m) {
    char tape[PM_TAPE_CELLS + 2];

    if (pm_machine_format(m, tape, (int)sizeof(tape)) != NULL) {
        printf("%s", tape);
    }
}

static void trace_line(const struct PmMachine *m, int line) {
    char text[32];

    pm_command_text(&m->lines[line - 1], text);
    printf("%7ld  %2d. %-9s ", m->step, line, text);
    print_tape(m);
    printf("\n");
}

static void usage(void) {
    fprintf(stderr, "использование: 10-post-machine [--program N | --file ФАЙЛ] [--input СЛОВО]\n"
                    "                               [--trace] [--limit K]\n"
                    "               10-post-machine --list\n");
}

int main(int argc, char **argv) {
    struct PmMachine *m;
    struct PmProgram from_file;
    const struct PmProgram *program;
    const char *file = NULL;
    const char *input = NULL;
    long limit = DEFAULT_LIMIT;
    int number = 1;
    int trace = 0;
    int code;
    int i;

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--program") == 0 && i + 1 < argc) {
            number = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--file") == 0 && i + 1 < argc) {
            file = argv[++i];
        } else if (strcmp(argv[i], "--input") == 0 && i + 1 < argc) {
            input = argv[++i];
        } else if (strcmp(argv[i], "--limit") == 0 && i + 1 < argc) {
            limit = atol(argv[++i]);
        } else if (strcmp(argv[i], "--trace") == 0) {
            trace = 1;
        } else if (strcmp(argv[i], "--list") == 0) {
            for (number = 0; number < pm_catalog_count(); ++number) {
                printf("%2d  %s — %s\n", number + 1, pm_catalog(number)->title,
                       pm_catalog(number)->group);
            }
            return 0;
        } else {
            usage();
            return 2;
        }
    }
    if (file != NULL) {
        if (!read_program(file, input, &from_file)) {
            return 2;
        }
        program = &from_file;
    } else {
        program = pm_catalog(number - 1);
        if (program == NULL) {
            fprintf(stderr, "post: программы с номером %d нет, всего %d (--list)\n", number,
                    pm_catalog_count());
            return 2;
        }
    }

    m = pm_machine_create();
    if (m == NULL) {
        fprintf(stderr, "post: не хватило памяти\n");
        return 1;
    }
    if (!pm_machine_load(m, program)) {
        fprintf(stderr, "post: %s: %s\n", program->title, m->error);
        pm_machine_destroy(m);
        return 2;
    }
    if (input != NULL && !pm_machine_set_input(m, input)) {
        fprintf(stderr, "post: %s\n", m->error);
        pm_machine_destroy(m);
        return 2;
    }

    printf("%s, слово «%s»\n", program->title, m->input);
    if (trace) {
        printf("   такт  команда       лента\n");
        printf("%7ld  %-13s ", 0L, "");
        print_tape(m);
        printf("\n");
        while (m->step < limit) {
            int line = m->line;
            if (!pm_machine_step(m)) {
                break;
            }
            trace_line(m, line);
        }
    } else {
        pm_machine_run(m, limit);
    }

    if (m->status == PM_RUNNING) {
        printf("не остановилась за %ld тактов, ", m->step);
    } else if (m->status == PM_HALTED) {
        printf("остановилась на такте %ld (строка %d), ", m->step, m->last_line);
    } else {
        printf("%s на такте %ld (строка %d), ", status_text(m->status), m->step, m->line);
    }
    printf("меток %d, лента «", pm_machine_count(m));
    print_tape(m);
    printf("»\n");
    code = (m->status == PM_HALTED) ? 0 : 1;
    pm_machine_destroy(m);
    return code;
}
