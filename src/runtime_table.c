/***********************************
TODO
- IMPORTANT: It's not immutable.
- IMPORTANT: Better data structure.
- String interning.
***********************************/

#include <stdlib.h>
#include <string.h>

#include "types.h"

#define REG_SIZE 8    // AMD64 Architecture

void initialize_table(table_t *table) {
	table->cap = 10;
	table->used = 0;
	table->data = malloc(table->cap * sizeof(symbol_t));
	table->stack_top = 0;
}

// NOTE(stefanos): Currently, we compile for 64bit architectures,
// meaning that by default, registers are 8 bytes (and so when
// we push them, we use 8 bytes from the stack memory).
void increment_stack_top(table_t *table) {
	table->stack_top += REG_SIZE;
}

void decrement_stack_top(table_t *table) {
	table->stack_top -= REG_SIZE;
}

int is_stack_aligned(table_t *table, int a) {
	return (table->stack_top % a == 0);
}

void insert(table_t *table, char *id, int line) {
	if(table->used >= table->cap) {
		table->cap *= 2;
		table->data = realloc(table->data, table->cap * sizeof(symbol_t));
	}
	table->data[table->used].id = id;
	table->data[table->used].line = line;
	increment_stack_top(table);
	table->data[table->used].offset = table->stack_top;
	table->used += 1;
}

int search(table_t *table, char *id) {
	for(int i = 0; i < table->used; ++i) {
		if(!strcmp(table->data[i].id, id))
			return i;
	}
	return -1;
}

void clean_table(table_t *table) {
	if(table && table->data)
		free(table->data);
}
