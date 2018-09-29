#include <stdlib.h>
#include "types.h"

void initialize_ast(ast_t *ast) {
	ast->root = NULL;
}

void add_statement(ast_t *ast, statement_t stat) {
	if(ast->root == NULL) {
		ast->root = malloc(sizeof(statement_node_t));
		ast->root->stat = stat;
		ast->root->next = NULL;
		ast->root->prev = NULL;
		return;
	}

	statement_node_t *traverse = ast->root;
	while(traverse->next != NULL) {
		traverse = traverse->next;
	}

	traverse->next = malloc(sizeof(statement_node_t));
	traverse->next->prev = traverse;
	traverse->next->stat = stat;
	traverse->next->next = NULL;
}

void printTabs(int tabs) {
	for(int i = 0; i < tabs; ++i)
		printf("\t");
}

void print_expression(exp_t *exp, int tabs) {
	if(exp->type == function_exp) {
		printTabs(tabs);
		printf("Function expression: \n");
		printTabs(tabs);
		printf("name: %s\n", exp->functionExp.name);
		printf("\n");
	} else if(exp->type == bin_exp) {
		printTabs(tabs);
		printf("Binary expression: \n");
		print_expression(exp->binExp.leftOperand, tabs + 1);
		printTabs(tabs);
		printf(" %s\n ", exp->binExp.operator);
		print_expression(exp->binExp.rightOperand, tabs + 1);
	} else if(exp->type == unary_exp) {
		printTabs(tabs);
		printf("unary exp: \n");
		printTabs(tabs);
		printf("operator: %s\n", exp->unaryExp.operator);
		printTabs(tabs);
		printf("operand: \n");
		print_expression(exp->unaryExp.operand, tabs + 1);
		printf("\n");
	} else if(exp->type == int_exp) {
		printTabs(tabs);
		printf("integer expression: \n");
		printTabs(tabs);
		printf("value: %d\n", exp->intExp);
	} else if(exp->type == id_exp) {
		printTabs(tabs);
		printf("id expression: \n");
		printTabs(tabs);
		printf("id: %s\n", exp->id);
	} else if(exp->type == assign_exp) {
		printTabs(tabs);
		printf("assignment expression: \n");
		printTabs(tabs);
		printf("id: %s\n", exp->id);
		printTabs(tabs);
		printf("rvalue: \n");
		print_expression(exp->assignExp.rvalue, tabs + 1);
	}
}

void print_statement(statement_t *stat, int tabs) {
	if(stat == NULL)
		return;
	if(stat->type == ret_stat) {
		printf("return statement:\n");
		print_expression(stat->retStat.exp, tabs + 1);
	} else if(stat->type == print_stat) {
		printf("print statement:\n");
		print_expression(stat->printStat.exp, tabs + 1);
	} else if(stat->type == break_stat) {
		printf("break statement\n");
	} else if(stat->type == cont_stat) {
		printf("continue statement\n");
	} else if(stat->type == decl_stat) {
		printf("Declaration statement:\n");
		printf("Id: %s\n", stat->declStat.id);
		printf("-----\n");
		printf("Value: \n");
		if(stat->declStat.rvalue != NULL)
			print_expression(stat->declStat.rvalue, tabs + 1);
	} else if(stat->type == simple_stat) {
		print_expression(stat->simpleStat.exp, tabs + 1);
	} else if(stat->type == if_stat) {
		printTabs(tabs);
		printf("If statement:\n");
		printTabs(tabs + 1);
		printf("Condition: \n");
		print_expression(stat->ifStat.cond, tabs + 2);
	} else if(stat->type == while_stat) {
		printTabs(tabs);
		printf("While statement:\n");
		printTabs(tabs + 1);
		printf("Condition: \n");
		print_expression(stat->whileStat.cond, tabs + 2);
	} else if(stat->type == else_stat) {
		printTabs(tabs);
		printf("Else statement:\n");
	} else if(stat->type == start_block) {
		printTabs(tabs);
		printf("\nStart of block: {\n\n");
	} else if(stat->type == end_block) {
		printTabs(tabs);
		printf("\nEnd of block: }\n\n");
	}
}

void print_ast(ast_t *ast) {
	statement_node_t *temp = ast->root;
	while(temp != NULL) {
		print_statement(&temp->stat, 0);
		temp = temp->next;
	}
}
