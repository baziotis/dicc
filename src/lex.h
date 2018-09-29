#ifndef LEX_H
#define LEX_H

#include "types.h"

lex_t *lex(char *filename);
void clean_lexer(lex_t *);

#endif
