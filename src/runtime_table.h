#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

void initialize_table(table_t *);
void increment_stack_top(table_t *);
int is_stack_aligned(table_t *, int);
void decrement_stack_top(table_t *);
void insert(table_t *, char *, int);
int search(table_t *, char *);
void clean_table(table_t *);

#endif
