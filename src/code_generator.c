#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "ast.h"
#include "utils.h"
#include "types.h"
#include "runtime_table.h"

// NOTE(stefanos): A note on code architecture. Throughout the project,
// I did error handling with having a single return point for a function,
// and having a success variable tracking whether we can continue at any point
// or not. That resulted in huge nested if-else statements, because we couldn't
// return at the point of failure. That has its good and bad parts. It is useful
// for functions that need to do something except for ONLY returning that it makes
// sense to try to have a single return point. Another solution to that is one
// that is even accepted in the Linux kernel code and that is, that the point of
// failure, we have a goto statement to the single return point. Finally, since
// here the function doesn't do anything special uppon failure apart from returning,
// I tried to experiment with returning at any failure point.

internal void next_statement(gen_t *gen) {
	if(gen && gen->curr_stat) {
		gen->curr_stat = gen->curr_stat->next;
	}
}

internal void prev_statement(gen_t *gen) {
	if(gen && gen->curr_stat) {
		gen->curr_stat = gen->curr_stat->prev;
	}
}

internal statement_t *peek_statement(gen_t *gen) {
	if(gen && gen->curr_stat) {
		return &(gen->curr_stat->stat);
	}
	return NULL;
}

/// Functions that have recursive dependencies
/// and need pre-declaration.
internal int assemble_block(gen_t *);
internal int assemble_statement(gen_t *gen);

internal void initialize_assembly(FILE *output) {

	// Initialize data segment with a pre-formatted string.
	// Only used by print statements.
	fprintf(output, ".data\n");
	fprintf(output, "fmt: .asciz \"int: %%d\\n\"\n");
	
	// Initialize text segment
	// Use Intel syntax
	fprintf(output, ".intel_syntax noprefix\n");
	// Align instructions to 4-byte boundary (not really needed)
	fprintf(output, ".align 4\n");
	fprintf(output, ".text\n");


	// function epilogue (same for every function for now)
	fprintf(output, ".func_epilogue:\n");
	fprintf(output, "mov rsp, rbp\n");
	fprintf(output, "pop rbp\n");
	fprintf(output, "ret\n");

}

internal void initialize_generator(gen_t *gen, ast_t *input, FILE *output) {
	gen->label = 1;
	gen->curr_loop_label = 0;
	gen->output = output;
	gen->curr_stat = input->root;
	initialize_table(&(gen->table));
}

internal int assemble_expression(gen_t *gen, exp_t *exp, int line) {
	FILE *output = gen->output;
	table_t *table = &(gen->table);

	if (exp->type == unary_exp) {
		if(!assemble_expression(gen, exp->unaryExp.operand, line))
			return 0;
		if (exp->unaryExp.operator[0] == '-')
			fprintf(output, "neg rax\n");
		else if (exp->unaryExp.operator[0] == '~')
			fprintf(output, "not rax\n");
		else if (exp->unaryExp.operator[0] == '!') {
			// Assume that 'eax' contains our non-yet-negated value.

			// We compare 'eax' with 0.
			fprintf(output, "cmp rax, 0\n");

			// Then, we want to zero 'eax' (so that
			// in the next instruction either keep it zero,
			// or set it to 1).
			// Zeroing with xor is faster than mov.
			fprintf(output, "xor rax, rax\n");
			
			// If the comparison set the ZF	(zero flag), i.e.
			// if 'eax' was 0, then we want to set its lower
			// byte (which we call with register %al) to 1 (thus
			// completing the negation).
			fprintf(output, "sete al\n");
		}
	} else if (exp->type == bin_exp) {
		// get the result from the left expression into eax
		if(!assemble_expression(gen, exp->binExp.leftOperand, line))
			return 0;
		// save the result on the stack
		fprintf(output, "push rax\n");
		increment_stack_top(table);
		
		// gethe result from the right expression into eax
		if(!assemble_expression(gen, exp->binExp.rightOperand, line))
			return 0;

		fprintf(output, "pop rcx\n");
		decrement_stack_top(table);
		
		char *operator = exp->binExp.operator;
		if (operator[0] == '+')
			fprintf(output, "add rax, rcx\n");
		else if (operator[0] == '*') {
			// NOTE(stefanos): We can use imul for multiplication,
			// but since we can show off and implement it using add
			// and shift, let's do it. You can check shift and add
			// method here: https://courses.cs.vt.edu/~cs1104/BuildingBlocks/multiply.040.html
			// Just note that I am doing in reverse order (we're starting from
			// the least significant bit).

			int tmp_lbl = gen->label;
			++(gen->label);

			// check sign of first operand.
			fprintf(output, "test rax, rax\n");
			fprintf(output, "js .signed_mult_%d\n", tmp_lbl);
			// check sign of second operand.
			fprintf(output, "test rcx, rcx\n");
			fprintf(output, "js .signed_mult_%d\n", tmp_lbl);
			
			// Unsigned multiplication
			fprintf(output, ".unsigned_mult_%d:\n", tmp_lbl);
			fprintf(output, "mov rdx, rax\n");  // first operand to rdx
			fprintf(output, "xor rax, rax\n");  // rax will be the result accumulator.
			fprintf(output, "mov rbx, rcx\n");  // second value to rbx  
			fprintf(output, "mov rcx, 0\n");   	// value to shift by
			// while(rdx != 0)
			fprintf(output, ".mult_loop_%d:\n", tmp_lbl);
			fprintf(output, "cmp rdx, 0\n");
			fprintf(output, "je .after_mult_%d\n", tmp_lbl);
			// if(rdx & 1) rax += rbx << rcx;
			fprintf(output, "mov r11, rdx\n");
			fprintf(output, "and r11, 1\n");
			fprintf(output, "cmp r11, 0\n");
			fprintf(output, "je .loop_end_%d\n", tmp_lbl);
			fprintf(output, "mov r11, rbx\n");
			fprintf(output, "shl r11, cl\n");
			fprintf(output, "add rax, r11\n");
			fprintf(output, ".loop_end_%d:\n", tmp_lbl);
			// ++rcx;
			fprintf(output, "add rcx, 1\n");
			// rdx >>= 1;
			fprintf(output, "shr rdx, 1\n");
			fprintf(output, "jmp .mult_loop_%d\n", tmp_lbl);
			
			// Signed multiplication
			fprintf(output, ".signed_mult_%d:\n", tmp_lbl);
			fprintf(output, "imul rax, rcx\n");

			fprintf(output, ".after_mult_%d:\n", tmp_lbl);
		} else if (operator[0] == '-') {
			// Remember that what we popped from the stack,
			// into rcx, was the FIRST value in the addition.
			// So, the subtraction is rcx - rax. But since we can't choose
			// where we save the result, it is automatically saved in the first
			// register and since we have the convention that the result goes
			// to eax, we have to save it there.
			fprintf(output, "sub rcx, rax\n");
			fprintf(output, "mov rax, rcx\n");
		} else if (operator[0] == '/') {
			// The command for integer division is: idiv dst
			// where 'dst' some register.
			// The division is computed as (rdx:rax) / dst
			// (be careful, 32 bit values)
			// and the quotient is saved in rax (obviously,
			// dst can't be either rax or rdx).

			// So, rcx has divisor and rax the dividend.

			// Move dividend to some temp register.
			fprintf(output, "mov rbx, rax\n");
			// Move divisor to rax
			fprintf(output, "mov rax, rcx\n");
			// extend the sign bit of rax to rdx
			fprintf(output, "cdq\n");
			fprintf(output, "idiv rbx\n");
		} else if (operator[0] == '%') {
			// Modulo is similar to division, because idiv saves the
			// quotient (the result of the division) in rax and the remainder
			// in rdx

			fprintf(output, "mov rbx, rax\n");
			// Move divisor to rax
			fprintf(output, "mov rax, rcx\n");
			// extend the sign bit of rax to rdx
			fprintf(output, "cdq\n");
			// remainder to rdx
			fprintf(output, "idiv rbx\n");
			// Move remainder to rax
			fprintf(output, "mov rax, rdx\n");
		} else if (operator[0] == '&') {
			// NOTE(stefanos): I do some special handling
			// for logical AND/OR, because x86 assembly
			// does not support logical AND/OR, only
			// bitwise.

			if (operator[1] == '&') {    // logical and
				fprintf(output, "\n");

				// dl = rcx != 0
				fprintf(output, "cmp rcx, 0\n");
				fprintf(output, "setne dl\n");

				// al = rax != 0
				fprintf(output, "cmp rax, 0\n");
				// For zeroing... First, you don't
				// want to do it after the setne, as
				// that will break the saved 'al' value,
				// and second, you don't want to do it with xor,
				// because that alters the eflags values.
				fprintf(output, "mov rax, 0\n");
				fprintf(output, "setne al\n");

				fprintf(output, "and al, dl\n");
			} else {    // bitwise and
			}
		} else if (operator[0] == '|') {
			if (operator[1] == '|') {    // logical or
				fprintf(output, "or rax, rcx\n");
				// zeroing not with xor, because
				// it alters the eflags values.
				fprintf(output, "mov rax, 0\n");
				fprintf(output, "setne al\n");
			} else {    // bitwise or
			}
		// TODO(stefanos): Maybe change that to use the
		// specific token types.
		} else if (strchr("!<=>", operator[0])) {
			// relational/equality expression
			fprintf(output, "cmp rcx, rax\n");
			fprintf(output, "mov rax, 0\n");
			if (operator[0] == '!') {
				// operator[1] has to be '='.
				fprintf(output, "setne al\n");
			} else if (operator[0] == '=') {
				// operator[1] has to be '='.
				fprintf(output, "sete al\n");
			} else if (operator[0] == '>') {
				if (operator[1] == '=')
					fprintf(output, "setge al\n");
				else
					fprintf(output, "setg al\n");
			} else if (operator[0] == '<') {
				if (operator[1] == '=')
					fprintf(output, "setle al\n");
				else
					fprintf(output, "setl al\n");
			}
		}
	} else if (exp->type == int_exp) {
		fprintf(output, "mov rax, %d\n", exp->intExp);
	} else if (exp->type == id_exp) {
		int index = search(table, exp->id);
		if (index == -1) {
			report_error(line, "Line %d: Undefined reference to identifier: %s\n", exp->id);
			return 0;
		}
		fprintf(output, "mov rax, [rbp - %d]\n", table->data[index].offset);
	} else if (exp->type == assign_exp) {
		if(!assemble_expression(gen, exp->assignExp.rvalue, line))
			return 0;
		int index = search(table, exp->assignExp.id);
		if (index == -1) {
			report_error(-1, "Undefined reference to identifier: %s\n", exp->assignExp.id);
			return 0;
		}
		fprintf(output, "mov [rbp - %d], rax\n", table->data[index].offset);
	}

	// assume success
	return 1;
}

internal int assemble_block(gen_t *gen) {
	statement_t *stat = peek_statement(gen);

	if (stat != NULL && stat->type == start_block) {
		next_statement(gen);
		stat = peek_statement(gen);
		while (stat != NULL && stat->type != end_block) {
			if(!assemble_statement(gen))
				return 0;
			stat = peek_statement(gen);
		}
		if (stat->type != end_block) {
			return 0;
		}
	}

	return 1;
}

internal int assemble_statement(gen_t *gen) {
	statement_t *stat = peek_statement(gen);
	assert(stat != NULL);
	
	FILE *output = gen->output;
	table_t *table = &(gen->table);
	
	int line = stat->line;   // line of statement

	if (stat->type == ret_stat) {
		if(!assemble_expression(gen, stat->retStat.exp, line))
			return 0;
		// jump function epilogue label relative to the function in which
		// this return statement is part of.
		fprintf(output, "jmp .func_epilogue\n");
	} else if(stat->type == break_stat) {
		if(gen->curr_loop_label == 0) {
			report_error(line, "Invalid break statement - not inside loop\n");
			return 0;
		}
		fprintf(output, "jmp .after_loop_%u\n", gen->curr_loop_label);
	} else if(stat->type == cont_stat) {
		if(gen->curr_loop_label == 0) {
			report_error(line, "Invalid continue statement - not inside loop\n");
			return 0;
		}
		fprintf(output, "jmp .loop_cond_%u\n", gen->curr_loop_label);
	} else if (stat->type == print_stat) {
		// result of the expression in rax.
		if(!assemble_expression(gen, stat->printStat.exp, line))
			return 0;
		
		// NOTE(stefanos):
		/*
			As we can see in the AMD64 ABI (Section 3.2.2 and end of Section 3.4.1),
			before process entry (so before a call instruction), %rsp has to be aligned
			to a 16-byte boundary (that is, it has to be a multiple on 16).
			Currently, stack is used in two places:
			1) Binary expressions.
			2) Variable declarations.

			In both of these cases, we count stack increase and decrease in the
			symbol table. So, we can push ANY general purpose register to align it.

			But why -8? CALL instruction pushes the RIP register onto the stack
			so that it where jump after the function finishes. Now, our main
			function is called by the c runtime library with... a CALL instruction.
			That means that before that CALL, RIP was pushed into the stack and
			so the stack is misaligned by 8.

			I want to point out that you can use this method for ALL functions,
			no matter how deep the call stack as they will have the same properties.
			(Namely, at their start the stack will be misaligned by 8 using the same
			logic).

			Finally, another solution is to insert assembly code that checks whether
			the stack is misaligned and pushes accordigly.
		*/
		int p = 0;
		if(!is_stack_aligned(table, 16 - 8)) {
			p = 1;
			fprintf(output, "push rbx\n");
			increment_stack_top(table);
		}
		fprintf(output, "lea rdi, fmt[rip]\n");
		// for printf, esi gets the result of the expression
		fprintf(output, "mov esi, eax\n");
		// In variable argument functions, like printf, AL is used
		// to indicate the number of vector arguments passed to a function
		// requiring a variable number of arguments. We have none, so we zero
		// EAX (and so AL which is its low byte).
		fprintf(output, "xor eax, eax\n");
		fprintf(output, "call printf\n");
		if(p) {
			fprintf(output, "pop rcx\n");
			decrement_stack_top(table);
		}
	} else if (stat->type == decl_stat) {
		char *id = stat->declStat.id;
		exp_t *rvalue = stat->declStat.rvalue;
		int index = search(table, id);
		if (index == -1) {
			insert(table, id, line);
			// default initialization to 0
			fprintf(output, "xor rax, rax\n");
			if (rvalue != NULL) {
				if(!assemble_expression(gen, rvalue, line))
					return 0;
			}
			// increment of stack_top in the symbol table is handled
			// by insert().
			fprintf(output, "push rax\n");
		} else {
			// variable already declared
			int id_line = table->data[index].line;
			report_error(line, "Variable %s is already declared in line %d\n", id, id_line);
			return 0;
		}
	} else if (stat->type == simple_stat) {
		if(!assemble_expression(gen, stat->simpleStat.exp, line))
			return 0;
	} else if (stat->type == else_stat) {
		// All valid else statements will be parsed
		// by the if statement parsing.
		report_error(line, "Unexpected else statement\n");
		return 0;
	} else if (stat->type == if_stat) {
		unsigned int tmp_lbl = gen->label;
		++(gen->label);

		// NOTE(stefanos): You can see previous commits
		// where I handle if statements in some different
		// ways. The main problem is that at the moment
		// we're generating the if statement, we don't
		// know if there is an else statement.

		// Assemble the condition
		if(!assemble_expression(gen, stat->ifStat.cond, line))
			return 0;
		// Now, we have the result of the condition in rax.
		fprintf(output, "cmp rax, 0\n");
		// False condition, jump to the after if code (that
		// can be either the else code, if there is one, or
		// the rest of the code)
		fprintf(output, "je .LIF1%d\n", tmp_lbl);
		// Assemble the if block
		next_statement(gen);
		if(!assemble_block(gen))
			return 0;
		if(peek_statement(gen)->type != end_block)
			return 0;

		// Assemble the else block
		next_statement(gen);
		stat = peek_statement(gen);
		if(stat != NULL) {
			if (stat->type == else_stat) {
				// There is else statement, so the after if code
				// is the else code (L1) and rest of code
				// the after else code (L2)

				// Part of the if code, at the end jump
				// to the rest of the code
				fprintf(output, "jmp .LIF2%d\n", tmp_lbl);
				// else code label
				fprintf(output, ".LIF1%d:\n", tmp_lbl);
				next_statement(gen);
				if(!assemble_block(gen))
					return 0;
				stat = peek_statement(gen);
				if(stat->type != end_block) {
					// TODO(stefanos): Could we ever have that error (because
					// of parenthesization check)?
					report_error(stat->line, "Expected end of block\n");
					return 0;
				}
				// rest of code label
				fprintf(output, ".LIF2%d:\n", tmp_lbl);
			} else {
				// We don't have else statement, so
				// the after if code is just the rest of the code.
				fprintf(output, ".LIF1%d:\n", tmp_lbl);
				prev_statement(gen);
			}
		}
	} else if (stat->type == while_stat) {
		unsigned int tmp_lbl = gen->label;
		unsigned int save_lbl = gen->curr_loop_label;
		++(gen->label);

		// Label condition
		fprintf(output, ".loop_cond_%u:\n", tmp_lbl);
		// Assemble the condition
		if(!assemble_expression(gen, stat->whileStat.cond, line))
			return 0;
		// Result of the condition in rax.
		fprintf(output, "cmp rax, 0\n");
		
		// False condition, jump to the .after_loop code code
		fprintf(output, "je .after_loop_%u\n", tmp_lbl);
		
		// Assemble the while block
		// Mark curent loop you're inside
		gen->curr_loop_label = tmp_lbl;
		next_statement(gen);
		if(!assemble_block(gen) || peek_statement(gen)->type != end_block)
			return 0;

		// Jump again to the condition
		fprintf(output, "jmp .loop_cond_%u\n", tmp_lbl);
		fprintf(output, ".after_loop_%u: \n",  tmp_lbl);

		// Done with this loop, fall back to previous (or none)
		gen->curr_loop_label = save_lbl;
	}

	next_statement(gen);

	// assume success
	return 1;
}

int assemble_function(gen_t *gen) {
	int success = 1;

	FILE *output = gen->output;
	statement_t *stat = peek_statement(gen);
	table_t *table = &(gen->table);

	if (stat->type == func_stat) {
		// Make the name of the function a global label.
		char *func_name = stat->funcStat.name;
		fprintf(output, ".globl %s\n", func_name);
		fprintf(output, "%s: \n", func_name);

		// function prologue (for every function)
		fprintf(output, "push rbp\n");
		fprintf(output, "mov rbp, rsp\n");

		while (peek_statement(gen) != NULL) {
			if(!assemble_statement(gen)) {
				return 0;
			}
		}

		// default return value 0
		fprintf(output, "xor eax, eax\n");
		fprintf(output, "jmp .func_epilogue\n");
	} else {
		report_error(-1, "Could not assemble function main\n");
		return 0;
	}

	// assume success
	return 1;
}

int generate(ast_t *input, FILE *output) {

	int ret;

	initialize_assembly(output);

	gen_t gen;
	initialize_generator(&gen, input, output);

	ret = assemble_function(&gen);

	clean_table(&(gen.table));

	return ret;
}
