#include <stdio.h>
#include <string.h>

#include "types.h"
#include "utils.h"
#include "lex.h"
#include "parser.h"
#include "ast.h"
#include "code_generator.h"

int main(int argc, char **argv) {

	FILE *dest;

	printf("-----------\n");
	printf("DICC: 0.0.1\n");
	printf("-----------\n\n");

	printf("-----------------[LEXER]----------------\n\n");
	lex_t *lex_output = lex(argv[1]); 

	if(!lex_output) {
		clean_lexer(lex_output);
		return 0;
	}

	ast_t *ast = parser(lex_output);
	if(!ast) {
		report_error(-1, "Could not generate AST\n");
		clean_lexer(lex_output);
		return 0;
	}
	printf("\n\n-----------------[PARSER]----------------\n\n");
	print_ast(ast);

	argv[1][strlen(argv[1]) - 1] = 's';
	dest = fopen(argv[1], "wb");

	if(!dest) {
		report_error(-1, "Destination file could not be opened for writing.\n");
		return 0;
	}

	printf("\n\n-----------------[CODE GENERATOR]----------------\n\n");
	generate(ast, stdout);
	generate(ast, dest);

	fclose(dest);
	clean_parser(ast);
	clean_lexer(lex_output);
	
	return 0;
}
