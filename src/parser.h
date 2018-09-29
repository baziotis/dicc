#ifndef PARSER_H
#define PARSER_H

#include "types.h"

ast_t *parser(lex_t *);
void clean_parser(ast_t *);

#endif
