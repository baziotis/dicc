#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "lex.h"
#include "utils.h"
#include "stack.h"

/**********
HOW IT WORKS:
Lexer is the simplest of the 3 phases and it just saves 2 kinds of tokens:
1) Anything that contains an alnum or _ is considered a word.
2) Everything else is considered some kind of symbol combination.
**********/

typedef struct reader {
	// TODO(stefanos): That's the maximum size of a word,
	// possibly this should be bigger.
	char buffer[256];
	int buffer_index;
	int in_comment;
	int lines;
	char *cp;
	stack_t *stack;
} reader_t;

// Read an entire file. Assume that if it fails, any
// footprint created, will be freed inside the function.
// TODO(George): I think that it might be a good idea to 
// enums for these kind of return values
internal int read_entire_file(const char *filename, read_file_t *file) {
	int success = 1;
	FILE *f = fopen(filename, "rb");
	
	if (f != NULL) {
		size_t filesize;

		// NOTE(stefanos): Possibly do a check both on the filename
		// and the filesize.
		fseek(f, 0, SEEK_END);   // Move to the end of the file
		filesize = ftell(f);     // Position of the file pointer relative
		// to the start of the file (in bytes).
		fseek(f, 0, SEEK_SET);   // Set file pointer to the start of the file.
		char *file_data = malloc((filesize + 1) * sizeof(char));
		
		if (!file_data) {
			report_error(-1, "Couldn't allocate memory for file data\n");
			success = 0;
		} else {
			fread(file_data, filesize, 1, f);
			file_data[filesize] = 0;
			file->filesize = filesize;
			file->file_data = file_data;
		}
		fclose(f);
	} else {
		success = 0;
		report_error(-1, "Failed to open the input file\n");
	}

	return success;
}

void clean_lexer(lex_t *output) {
	if (output) {
		if (output->token_num) {
			for (int i = 0; i != output->token_num; ++i)
				free(output->token[i].tok);
			free(output->token);
		}
		free(output);
		
		if (output->file_data) {
			// Assume that it's not corrupted memory.
			free(output->file_data);
		}
	}
}


// Checks if a string an integer constant.
internal int is_constant(char *buffer) {
	while (*buffer) {
		if (!isdigit(*buffer))
			return 0;
		++buffer;
	}
	return 1;
}

internal int is_data_type(const char *buffer)
{
	return (
		!strcmp("int", buffer) ||
		!strcmp("long", buffer) ||
		!strcmp("float", buffer) ||
		!strcmp("double", buffer) ||
		!strcmp("void", buffer)
	);
}

internal int is_keyword(const char *buffer)
{
	return (
		!strcmp("if", buffer) ||
		!strcmp("else", buffer) ||
		!strcmp("print", buffer) ||   // IMPORTANT(stefanos): This keyword is mine!
		!strcmp("for", buffer) ||
		!strcmp("while", buffer) ||
		!strcmp("do", buffer) ||
		!strcmp("break", buffer) ||
		!strcmp("return", buffer) ||
		!strcmp("continue", buffer)
	);
}

internal int is_valid_id(const char *buffer) {
	// NOTE(stefanos): For a complete check on a valid
	// ID, we have to check if it is a reserved word.
	// However, in lexing part, if it is, it will
	// be signified in what_type() as keyword.
	// So, for a complete check, we have
	// to verify in the parser that where
	// we expect an ID, we don't get a
	// keyword.


	// First character should be alphabetical
	// or underscore.
	if(!(isalpha(buffer[0]) || buffer[0] == '_'))
		return 0;

	++buffer;
	// Rest of characters should be alphanumerical
	// or underscore.
	while(buffer[0] != '\0') {
		if(!(isalnum(buffer[0]) || buffer[0] == '_'))
			return 0;
		++buffer;
	}

	return 1;
}

// Determines the token type of buffer, where
// buffer contains a token.
internal int what_type(char *buffer) {
	if(buffer[0] == '\0')
		return UNKNOWN;

	switch (buffer[0]) {
		case '(': return LPAR;
		case ')': return RPAR;
		case '{': return LBRACE;
		case '}': return RBRACE;
		case '[': return LBRACKET;
		case ']': return RBRACKET;
		case ';': return SEMICOLON;
		case '+': return PLUS;
		case '-': return MINUS;
		case '*': return STAR;
		case '"': return DQUOTE;
		case '/': return SLASH;
		case '%': return MOD;
		case '~': return UOPERATOR;
		case '<': return (buffer[1] == '=') ? LE : LT;
		case '>': return (buffer[1] == '=') ? GE : GT;
		case '&': if (buffer[1] == '&') return LAND; break;
		case '|': if (buffer[1] == '|') return LOR; break;
		case '=': return (buffer[1] == '=') ? EQ : ASSIGN;
		case '!': return (buffer[1] == '=') ? NEQ : UOPERATOR;
	}

	if (is_data_type(buffer)) {
		return DATA_TYPE;
	} else if (is_keyword(buffer)) {
		return KEYWORD;
	} else if (is_constant(buffer)) {
		return CONSTANT;
	} else if(is_valid_id(buffer)) {
			return IDENTIFIER;
	}

	return UNKNOWN;
}

internal int save_token(lex_t *result, reader_t *reader) {
	int type;

	result->token = realloc(result->token, (result->token_num + 1) * sizeof(token_t));
	
	reader->buffer[reader->buffer_index] = '\0';
	// TODO - IMPORTANT (stefanos): That should AT LEAST follow the rule "when
	// run out of space, allocate double the current size"
	result->token[result->token_num].tok = malloc(reader->buffer_index * sizeof(char));
	result->token[result->token_num].line = reader->lines;
	type = what_type(reader->buffer);
	if(type == UNKNOWN) {
		report_error(reader->lines, "Unknown token\n");
		return 0;
	} else if(type == IDENTIFIER) {
		if(!is_valid_id(reader->buffer)) {
			report_error(reader->lines, "Invalid identifier: %d %s\n", strlen(reader->buffer), reader->buffer);
			return 0;
		}
	}
	result->token[result->token_num].type = type;
	strcpy(result->token[result->token_num].tok, reader->buffer);
	
	printf("%s\n", reader->buffer);
	
	++(result->token_num);
	reader->buffer_index = 0;
}

// 1: Not in comment
// 0: In comment
internal int check_comment(reader_t *reader) {
	int success = 1;

	// check for start of comment
	if (reader->cp[0] == '/') {
		if (reader->cp[1] == '*') {
			if (!reader->in_comment) {
				reader->cp += 2;
				reader->in_comment = 1;
			} else {
				report_error(reader->lines, "Nested comments are not allowed\n");
				success = 0;
			}
		}
	}

	// check for end of comment
	if (reader->cp[0] == '*') {
		if (reader->cp[1] == '/') {
			if (reader->in_comment) {
				reader->cp += 2;
				reader->in_comment = 0;
			} else {
				report_error(reader->lines, "Invalid use of end comment '*/' token\n");
				success = 0;
			}
		}
	}

	return success;
}

internal int is_left_parenthesization(char c)
{
	return (c == '(' || c == '{' || c == '[');
}

internal int is_right_parenthesization(char c)
{
	return (c == ')' || c == '}' || c == ']');
}

internal int are_pair(char left, char right)
{
	if (left == '(' && right == ')') return 1;
	else if (left == '{' && right == '}') return 1;
	else if (left == '[' && right == ']') return 1;
	return 0;
}

/*********** TODO *************
1) Lexer should (at least) create the symbol table
for variables and for constants save their value.

2) Lexer could be much more intelligent
and do things like save the value of constants,
size of strings etc.
******************************/

lex_t *lex(char *filename) {
	lex_t *result = malloc(sizeof(lex_t));
	if (result == NULL) {
		report_error(-1, "Out of memory");
		return NULL;
	}

	result->token_num = 0;
	result->token = NULL;

	if (!read_entire_file(filename, &result->file_contents))
		return NULL;

	reader_t reader = {
		.buffer_index = 0,
		.in_comment = 0,
		.stack = create_stack(),
		.lines = 1
	};
	reader.buffer[0] = '\0';

	char c;
	char second;
	
	int in_word = 0;
	int success = 1;
	for (reader.cp = result->file_data; success && reader.cp[0]; ++(reader.cp)) {
		success = check_comment(&reader);

		if (!success)
			break;

		if (reader.in_comment)
			continue;

		// get the current character
		c = reader.cp[0];

		if (is_left_parenthesization(c)) {
			push_into_stack(reader.stack, c);
		} else if (is_right_parenthesization(c)) {
			if (check_stack_empty(reader.stack) ||
				!are_pair(peek_top_of_stack(reader.stack), c)) {
				success = 0;
				report_error(-1, "Incorrect parenthesization\n");
				break;
			} else {
				pop_from_stack(reader.stack);
			}
		}

		if (isalnum(c) || c == '_') {
			// part of a word
			if (!in_word)
				in_word = 1;
			reader.buffer[(reader.buffer_index)++] = c;
		} else if (!isspace(c)) {
			if (in_word) {
				in_word = 0;
				// TODO(stefanos): Write this more cleanly!
				success = (save_token(result, &reader) != UNKNOWN) 
					? success : 0;
				if(!success)
					break;
			}
			
			reader.buffer[(reader.buffer_index)++] = c;
			
			
			// Handle length 1 operators which can also be part
			// of length 2 operators.
			second = 0;
			if (c == '&' || c == '|')
				second = c;
			else if (c == '!' || c == '=' || c == '<' || c == '>')
				second = '=';
			
			if (second) {
				if (reader.cp[1] == second) {
					++(reader.cp);
					reader.buffer[(reader.buffer_index)++] = second;
				}
			}
			success = (save_token(result, &reader) != UNKNOWN) 
					? success : 0;
		} else {
			if(c == '\n')
				(reader.lines)++;
			if (reader.buffer_index) {
				// whatever we have saved (if we have saved) thus far,
				// we consider it a token.
				if (in_word)
					in_word = 0;
				success = (save_token(result, &reader) != UNKNOWN) 
					? success : 0;
			}
		}
	}
	
	if (success && !check_stack_empty(reader.stack)) {
		report_error(-1, "Unexpected end of input - Incorrect parenthesization\n",
			reader.lines);
		success = 0;
	}

	free_stack(reader.stack);

	if (success)
		return result;
	else
		return NULL;
}
