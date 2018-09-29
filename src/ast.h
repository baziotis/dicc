#ifndef AST_H
#define AST_H

#include "types.h"

void initialize_ast(ast_t *);
void add_statement(ast_t *, statement_t);
void print_expression(exp_t *, int);
void print_statement(statement_t *, int);
void print_ast(ast_t *);

#endif
