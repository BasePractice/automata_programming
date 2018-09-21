#include <stdlib.h>
#include <stdio.h>
#include "turing_machine.h"

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
	fprintf(stdout, "%s\n", engine_type_get(engine, offset));
	machine(engine);
	engine_tape_set(engine, offset + (int)strlen(TEXT), (char)0);
	fprintf(stdout, "%s\n", engine_type_get(engine, offset));
	engine_destroy(&engine);
#undef TEXT
    system("pause");
    return EXIT_FAILURE;
}
