#ifndef TYPES_H
#define TYPES_H

#include <stdio.h>

#define internal static
#define global_var static

/*** SYMBOLS ***/
enum { 
UNKNOWN, DATA_TYPE, KEYWORD, CONSTANT, IDENTIFIER, ASSIGN, DQUOTE, PLUS,
MINUS, STAR, SLASH, MOD, LT, LE, GT, GE, EQ, NEQ, LAND, LOR, UOPERATOR,
LPAR, RPAR, LBRACE, RBRACE, LBRACKET, RBRACKET, SEMICOLON
};

/********* LEXER *********/

typedef struct {
	char *file_data;
	size_t filesize;   // in bytes
} read_file_t;

typedef struct {
	char *tok;
	int line;
	int type;
} token_t;

typedef struct {
	int token_num;
	token_t *token;
	union {
		struct {
			char *file_data;
			size_t filesize;   // in bytes
		};
		
		read_file_t file_contents;
	};
} lex_t;


/********* PARSER *********/

typedef struct {
	int token_index;
	lex_t *input;
	int blocks;   // number of nested blocks we are
				  // currently in.
} parser_t;


typedef struct exp {
	enum { int_exp, id_exp, unary_exp, function_exp, bin_exp, decl_exp, assign_exp } type;
	union { 
		int intExp;
		
		char *id;

		struct {
			char *operator;
			struct exp *operand;
		} unaryExp;

		struct {
			char *operator;
			struct exp *leftOperand;
			struct exp *rightOperand;
		} binExp;
		
		struct {
			char *name;
		} functionExp;

		struct {
			char *id;
			struct exp *rvalue;
		} assignExp;
	};
} exp_t;

// That eventually should become a list of functions.
typedef struct statement {
	enum { func_stat, simple_stat, ret_stat, decl_stat, if_stat, else_stat, 
		break_stat, cont_stat, print_stat, while_stat, start_block, end_block} type;
	int line;    // for error reporting
	union {
		struct {
			char *name;
		} funcStat;

		struct {
			exp_t *exp;
		} retStat, printStat, unaryStat;
		// unaryStat is added as a general type
		// for statements that have only one expression.

		struct {
			char *id;
			exp_t *rvalue;
		} declStat;

		struct {
			exp_t *cond;
		} ifStat, whileStat;
		
		struct {
			exp_t *exp;
		} simpleStat;
	};
} statement_t;

/********* AST *********/

// Maybe not implement it as a list
// (stefanos): Changing that to an array
// may do our lives a lot easier.
typedef struct statement_node {
	statement_t stat;
	struct statement_node *next;
	struct statement_node *prev;
} statement_node_t;

typedef struct ast {
	statement_node_t *root;
} ast_t;



/********* GENERATOR ************/

/********* SYMBOL TABLE *********/
// NOTE(stefanos): Remember that the runtime stack
// grows towards lower addresses. That means that every time
// we push something to the stack, its address is lower relative
// to %rbp. So if a variable has an offset of 8 (the offset we save),
// then its value is at %rbp - 8;
typedef struct {
	char *id;
	int line;     // line of declaration
	int offset;   // relative to %rbp
} symbol_t;

typedef struct {
	int cap;
	int used;
	symbol_t *data;
	int stack_top;  // relative to %rbp
} table_t;

typedef struct gen {
	FILE *output;
	statement_node_t *curr_stat;   // TODO(stefanos): Bad name...
	table_t table;
	unsigned int label;    		   // general-purpose labels
	unsigned int curr_loop_label;  // label of the loop we're currently in.
} gen_t;

#endif
