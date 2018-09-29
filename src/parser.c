#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "assert.h"

#include "types.h"
#include "utils.h"
#include "ast.h"


/// Functions that have recursive dependences and
/// need pre-declaration.
internal int parse_expression(parser_t *, exp_t *);

internal void initialize_parser(parser_t *parser, lex_t *lex_input) {
	parser->input = lex_input;
	parser->token_index = 0;
	
	// TODO(stefanos): This is probably a very bad solution.
	parser->blocks = 0;  // how many nested blocks we are.
}

internal void clean_expression(exp_t *exp) {
	if (exp->type == bin_exp) {
		clean_expression(exp->binExp.leftOperand);
		free(exp->binExp.leftOperand);
		clean_expression(exp->binExp.rightOperand);
		free(exp->binExp.rightOperand);
	} else if (exp->type == unary_exp) {
		clean_expression(exp->unaryExp.operand);
		free(exp->unaryExp.operand);
	} else if (exp->type == assign_exp) {
		clean_expression(exp->assignExp.rvalue);
		free(exp->assignExp.rvalue);
	}
}

internal void clean_statement(statement_t *stat) {
	if (stat->type == ret_stat) {
		clean_expression(stat->retStat.exp);
		free(stat->retStat.exp);
	} else if (stat->type == print_stat) {
		clean_expression(stat->printStat.exp);
		free(stat->printStat.exp);
	} else if (stat->type == decl_stat) {
		if (stat->declStat.rvalue != NULL) {
			clean_expression(stat->declStat.rvalue);
			free(stat->declStat.rvalue);
		}
	} else if (stat->type == simple_stat) {
		clean_expression(stat->simpleStat.exp);
		free(stat->simpleStat.exp);
	} else if (stat->type == if_stat) {
		clean_expression(stat->ifStat.cond);
		free(stat->ifStat.cond);
	} else if (stat->type == while_stat) {
		clean_expression(stat->whileStat.cond);
		free(stat->whileStat.cond);
	}
}

void clean_parser(ast_t *ast) {
	if (ast) {
		statement_node_t *save;
		statement_node_t *temp = ast->root;
		while (temp != NULL) {
			clean_statement(&temp->stat);
			save = temp->next;
			free(temp);
			temp = save;
		}

		free(ast);
	}
}

// Determines the token type of buffer, where
// buffer contains a token.
internal inline int what_type(token_t token) {
	return token.type;
}

// Returns a pointer to the current token
// without advancing to the next
internal token_t peek_token(parser_t *parser) {
	assert(parser->token_index < parser->input->token_num);
	token_t next_token = parser->input->token[parser->token_index];
	return next_token;
}

internal int curr_line(parser_t *parser) {
	return parser->input->token[parser->token_index].line;
}

// Same as peek_token(), it just advances to
// the next token.
internal token_t get_token(parser_t *parser) {
	token_t next_token = peek_token(parser);
	parser->token_index += 1;
	return next_token;
}

// Moves back a token.
internal void unget_token(parser_t *parser) {
	if (parser->token_index)
		parser->token_index -= 1;
}

internal int parse_unit(parser_t *parser, exp_t *output) {
	int success = 1;
	token_t next_token = get_token(parser);

	int token_type = what_type(next_token);
	if (token_type == CONSTANT) {
		output->type = int_exp;
		// convert the (integer) constant expression
		output->intExp = atoi(next_token.tok);

	} else if (token_type == IDENTIFIER) {
		output->type = id_exp;
		output->id = next_token.tok;
	} else if (token_type == UOPERATOR || token_type == MINUS) {
		// unary operator means unary expression.
		output->type = unary_exp;
		output->unaryExp.operator = next_token.tok;
		// the operand can be a whole expression itself,
		// so take space.
		output->unaryExp.operand = malloc(sizeof(exp_t));

		if (!parse_unit(parser, output->unaryExp.operand)) {
			success = 0;
		}

	} else if (token_type == LPAR) {
		// Treat what's inside as a whole expression.
		if (!parse_expression(parser, output))
			success = 0;

		// Expect a closing (right) paren at the end
		next_token = get_token(parser);
		if (what_type(next_token) != RPAR)
			success = 0;
	}

	return success;
}

internal int type_is_one_of(int type, int *types, int size) {
	for (int i = 0; i != size; ++i) {
		if (type == types[i])
			return 1;
	}
	return 0;
}

// All binary expressions have the same structure.
// They first have a left binary expression of higher precedence
// and then, possibly, more right binary expressions, also of higher
// precedence, which are concatenated with some symbol, depending
// on the binary expression. Note that this goes somewhat against
// the intuition that binary expressions must have two components.
// However, this definition comes with some benefits.
// It handles precedence and associativity rules correctly.
// Finally, the precedence level is handled by the fact that
// parsing functions for different expressions are called
// from lower precedence to highest.
/*******
Example:
1 + 2 * 3

Here, first parse_additive_exp will be called and it will try to find
a left expression, which is 1. Then, it will see the + symbol and that
means we _must_ have a right expression, which is 2 * 3. Recursively,
for the right expression, parse_factor_exp will be called (whose work
is very easy). So, the AST will be:
Binary expression:
	left: 1
		+
	right: Binary Expression
		left: 2
			*
		right: 3
*******/
internal int parse_binary_exp(parser_t *parser, exp_t *output, int *types, int types_size, int (*higher_prec_exp)(parser_t *, exp_t *)) {
	token_t next_token;
	int success = 1;

	// You gotta have at least one higher precedence
	// (left) expression.
	if (!higher_prec_exp(parser, output)) {
		success = 0;
	}

	next_token = peek_token(parser);
	int type = what_type(next_token);
	// symbol is one of the available
	while (type_is_one_of(type, types, types_size)) {
		// there are other terms
		next_token = get_token(parser);

		// construct the right term
		exp_t *right = malloc(sizeof(exp_t));
		int res = higher_prec_exp(parser, right);

		// You have to do this first (before change its
		// type and stuff)
		// The new left term is the current term.
		exp_t *temp = malloc(sizeof(exp_t));
		*temp = *output;
		
		// Construct the binary expression.
		output->type = bin_exp;
		output->binExp.operator = next_token.tok;
		output->binExp.leftOperand = temp;
		output->binExp.rightOperand = right;

		if (!res) {
			success = 0;
			break;
		}
		
		next_token = peek_token(parser);
		type = what_type(next_token);
	}

	return success;
}

internal int parse_factor_exp(parser_t *parser, exp_t *output) {
	int types[3] = {STAR, SLASH, MOD};
	return parse_binary_exp(parser, output, types, 3, parse_unit);
}

internal int parse_additive_exp(parser_t *parser, exp_t *output) {
	int types[2] = {PLUS, MINUS};
	return parse_binary_exp(parser, output, types, 2, parse_factor_exp);
}

internal int parse_relational_exp(parser_t *parser, exp_t *output) {
	int types[4] = {LT, LE, GT, GE};
	return parse_binary_exp(parser, output, types, 4, parse_additive_exp);
}

internal int parse_equality_exp(parser_t *parser, exp_t *output) {
	int types[2] = {EQ, NEQ};
	return parse_binary_exp(parser, output, types, 2, parse_relational_exp);
}

internal int parse_logical_and_exp(parser_t *parser, exp_t *output) {
	int types[1] = {LAND};
	return parse_binary_exp(parser, output, types, 1, parse_equality_exp);
}

internal int parse_logical_or_exp(parser_t *parser, exp_t *output) {
	int types[1] = {LOR};
	return parse_binary_exp(parser, output, types, 1, parse_logical_and_exp);
}

// Parse an expression
internal int parse_expression(parser_t *parser, exp_t *output) {
	int success = 1;
	token_t next_token = peek_token(parser);

	// Assignment expression.
	if (what_type(next_token) == IDENTIFIER) {
		char *id = next_token.tok;
		get_token(parser);
		next_token = peek_token(parser);
		if (what_type(next_token) == ASSIGN) {
			// assume assignment expression

			next_token = get_token(parser);
			output->type = assign_exp;
			output->assignExp.id = id;

			exp_t *temp_exp = malloc(sizeof(exp_t));
			if (!parse_logical_or_exp(parser, temp_exp)) {
				//printf("failed\n");
				success = 0;
			}

			output->assignExp.rvalue = temp_exp;
			success = 1;
		} else {
			// other expression
			unget_token(parser);
			success = parse_logical_or_exp(parser, output);
		}
	// Other kind of expression.
	} else {
		success = parse_logical_or_exp(parser, output);
	}

	return success;
}

// Require that the next token
// is a semicolon.
internal int require_semicolon(parser_t *parser) {
	token_t next_token = get_token(parser);
	if (what_type(next_token) != SEMICOLON) {
		return 0;
	}
	return 1;
}

/*******
NOTE(stefanos):
	I don't know if the term unary statement even exists,
	but just for ease of use, these are statements that have only
	one expression after them, like return and print.
*******/
internal int unary_statement(statement_t *output, char *keyword, parser_t *parser) {

	int success = 1;
	int res;
		
	get_token(parser);

	if (!strcmp(keyword, "return"))
		output->type = ret_stat;
	else if (!strcmp(keyword, "print"))
		output->type = print_stat;

	exp_t *temp_exp = malloc(sizeof(exp_t));
	res = parse_expression(parser, temp_exp);

	output->unaryStat.exp = temp_exp;
	if (res) {
		res = require_semicolon(parser);
		if (!res) {
			report_error(curr_line(parser),
				"No semicolon after the %s expression\n", keyword);
			success = 0;
		}
	} else {
		report_error(curr_line(parser), 
			"Invalid %s expression\n", keyword);
		success = 0;
	}

	return success;
}

internal int no_op_statement(parser_t *parser, char *keyword, statement_t *output) {
	int res;
	if(!strcmp(keyword, "break"))
		output->type = break_stat;
	else if(!strcmp(keyword, "continue"))
		output->type = cont_stat;
	else
		return 0;

	get_token(parser);
	res = require_semicolon(parser);
	if (!res) {
		report_error(curr_line(parser), "No semicolon in no op statement\n");
		return 0;
	}

	return 1;
}

// Parse one statement
internal int parse_statement(parser_t *parser, statement_t *output) {
	token_t next_token;
	char *tok_str;
	int success = 1;
	int res;

	next_token = peek_token(parser);
	tok_str = next_token.tok;

	output->line = curr_line(parser);
	
	if (what_type(next_token) == LBRACE) {
		get_token(parser);
		parser->blocks += 1;
		output->type = start_block;
	} else if (what_type(next_token) == RBRACE) {
		get_token(parser);
		parser->blocks -= 1;
		output->type = end_block;
	} else if (what_type(next_token) == KEYWORD && !strcmp(tok_str, "return")) {
		// return statement

		if (!unary_statement(output, "return", parser))
			success = 0;

	} else if (what_type(next_token) == KEYWORD && !strcmp(tok_str, "print")) {
		// Print statement
		
		/***
		I made up a simple print statement so that programs can print
		something for debug purposes.
		The format is: print e;
		where 'e' is some (integer) expression.
		***/

		if (!unary_statement(output, "print", parser))
			success = 0;
		
	} else if (what_type(next_token) == KEYWORD && !strcmp(tok_str, "if")) {
		// if statement
		// parse another statement for the if
		get_token(parser);
		next_token = get_token(parser);
		
		output->type = if_stat;

		if (what_type(next_token) == LPAR) {
			exp_t *temp_exp = malloc(sizeof(exp_t));
			res = parse_expression(parser, temp_exp);
			output->ifStat.cond = temp_exp;
			if (res) {
				next_token = get_token(parser);
				if (what_type(next_token) != RPAR) {
					report_error(curr_line(parser), "Missing right paren in the if\n");
					success = 0;
				}
			} else {
				report_error(curr_line(parser), "Invalid condition expression in the if\n");
				success = 0;
			}
		} else {
			report_error(curr_line(parser), "Missing left paren in the if\n");
			success = 0;
		}
	} else if (what_type(next_token) == KEYWORD && !strcmp(tok_str, "else")) {
		get_token(parser);
		output->type = else_stat;
	} else if (what_type(next_token) == KEYWORD &&
				(!strcmp(tok_str, "break") || !strcmp(tok_str, "continue"))) {
		if(!no_op_statement(parser, next_token.tok, output))
			success = 0;
	} else if (what_type(next_token) == KEYWORD && !strcmp(tok_str, "while")) {
		// while statement
		// similar to if
		get_token(parser);
		next_token = get_token(parser);
		
		output->type = while_stat;

		if (what_type(next_token) == LPAR) {
			exp_t *temp_exp = malloc(sizeof(exp_t));
			res = parse_expression(parser, temp_exp);
			output->whileStat.cond = temp_exp;
			if (res) {
				next_token = get_token(parser);
				if (what_type(next_token) != RPAR) {
					report_error(curr_line(parser),
						"Missing right paren in the while, got: %s\n", next_token);
					success = 0;
				}
			} else {
				report_error(curr_line(parser), "Invalid condition expression in the while\n");
				success = 0;
			}
		} else {
			report_error(curr_line(parser), "Missing left paren in the while\n");
			success = 0;
		}

	} else if (what_type(next_token) == DATA_TYPE) {
		// Declaration statement

		next_token = get_token(parser);

		output->type = decl_stat;

		// Next token should be indentifier
		next_token = get_token(parser);

		if (what_type(next_token) == IDENTIFIER) {
			output->declStat.id = next_token.tok;

			next_token = peek_token(parser);  // either ';' or '='
			// i.e. either we have initialization or not.


			int type = what_type(next_token);
			get_token(parser);
			if (type == ASSIGN) {
				// We _must_ have an rvalue.
				exp_t *temp_exp = malloc(sizeof(exp_t));
				res = parse_expression(parser, temp_exp);

				output->declStat.rvalue = temp_exp;
				
				// next token should be semicolon and we're finished.
				if (res) {
					res = require_semicolon(parser);
					if (!res) {
						report_error(curr_line(parser), "No semicolon in the declaration\n");
						success = 0;
					}
				} else {
					success = 0;
				}
			} else if (type == SEMICOLON) {
				// We don't have an rvalue.
				output->declStat.rvalue = NULL;
			} else {
				report_error(curr_line(parser), 
					"Invalid declaration statement: %s\n", 
					output->declStat.id);
				success = 0;
			}
		} else {
			success = 0;
		}
	} else {
		output->type = simple_stat;

		exp_t *temp_exp = malloc(sizeof(exp_t));
		res = parse_expression(parser, temp_exp);

		output->simpleStat.exp = temp_exp;

		// next token should be semicolon and we're finished.
		if (res) {
			res = require_semicolon(parser);
			if (!res) {
				report_error(curr_line(parser), "No semicolon in the simple statement\n");
				success = 0;
			}
		} else {
			success = 0;
		}
	}

	return success;
}

// Parse one function
internal int parse_function(parser_t *parser, ast_t *ast) {
	statement_t func;
	statement_t result;
	int success = 1;

	token_t next_token;

	func.type = func_stat;
	
	// Next token should be
	// a data type (function's return type)
	next_token = get_token(parser);
	if (what_type(next_token) != DATA_TYPE) {
		printf("%s\n", peek_token(parser).tok);
		report_error(curr_line(parser), "Expected data type\n");
		return 0;
	}

	// Next token should be 
	// an identifier (function's name)
	next_token = get_token(parser);
	if (what_type(next_token) != IDENTIFIER) {
		report_error(curr_line(parser), "Expected identifier\n");
		return 0;
	}

	
	char *name = next_token.tok;   // Save the name for error messages.
	// Copy the function's name
	func.funcStat.name = name;
	add_statement(ast, func);  // add it before the statements.

	// Next token should be an open (left) paren
	if (what_type(get_token(parser)) != LPAR) {
		report_error(curr_line(parser), 
			"Expected left paren while parsing function: %s\n", name);
		return 0;
	}

	// Next token should be a closing paren
	if (what_type(get_token(parser)) != RPAR) {
		report_error(curr_line(parser), 
			"Expected right paren while parsing function: %s\n", name);
		return 0;
	}

	// Next token should be {
	if (what_type(get_token(parser)) != LBRACE) {
		report_error(curr_line(parser), 
			"Expected left brace while parsing function: %s\n", name);
		return 0;
	}

	while (!(what_type(peek_token(parser)) == RBRACE && parser->blocks == 0)) {
		int res = parse_statement(parser, &result);

		// IMPORTANT(stefanos): We add the statement anyway
		// so the freeing of any inside expressions
		// works correctly.
		add_statement(ast, result);
		if (!res) {
			success = 0;
			break;
		}
	}
	
	return success;
}

ast_t *parser(lex_t *input) {

/************ GENERAL TODO s ***********************
1) String interning.
***************************************************/

	parser_t parser;
	initialize_parser(&parser, input);
	
	ast_t *ast = malloc(sizeof(ast_t));
	initialize_ast(ast);

	// Parse function
	if (!parse_function(&parser, ast)) {
		report_error(-1, "Failed to parse function\n");
		clean_parser(ast);
		return NULL;
	}

	return ast;
}
