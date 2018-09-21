#include <stdlib.h>
#include <stdio.h>
#include "turing_machine.h"

void print_type(struct Engine *engine) {
	int i;

	for (i = 0; i < TAPE_LIMIT; ++i) {
		int ch = *(engine_type_get(engine, i));
		if (ch == EMPTY_SYMBOL)
			continue;
		fprintf(stdout, "%c", (char)ch);
	}
	fprintf(stdout, "\n");
	fflush(stdout);
}

int
main(int argc, char *argv[]) {
	struct Engine * engine = engine_create(3, 2, 'A');

	engine_symbol_add(engine, '0');
	engine_symbol_add(engine, '1');
	engine_symbol_add(engine, EMPTY_SYMBOL);

	engine_state_add(engine, 'A');
	engine_state_add(engine, 'B');

	engine_reference_add(engine, '0', 'A', '1', 'B', Right);
	engine_reference_add(engine, '1', 'A', '0', 'B', Right);
	engine_reference_add(engine, EMPTY_SYMBOL, 'A', EMPTY_SYMBOL, 'A', Right);

	engine_reference_add(engine, '0', 'B', '1', 'B', Right);
	engine_reference_add(engine, '1', 'B', '0', 'B', Right);
	engine_reference_add(engine, EMPTY_SYMBOL, 'B', EMPTY_SYMBOL, STOP_STATE, Right);

#define TEXT "110010"
	int offset = (TAPE_LIMIT / 2) + 2;
	engine_tape_copy(engine, offset, TEXT);
	print_type(engine);
	machine(engine);
	print_type(engine);
	engine_destroy(&engine);
#undef TEXT
    system("pause");
    return EXIT_FAILURE;
}
