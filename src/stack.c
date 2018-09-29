#include <stdlib.h>
#include "types.h"

// NOTE:(George) I assume that malloc won't return NULL for a node allocation
// maybe I should check on that

struct stack_node {
	struct stack_node *previous;
	char character;
};

struct stack {
	struct stack_node *top;
	size_t number_of_nodes;
};

internal inline struct stack_node *create_stack_node(char character)
{
	struct stack_node *new_stack_node = malloc(sizeof(struct stack_node));
	new_stack_node->previous = NULL;
	new_stack_node->character = character;
	return new_stack_node;
}

struct stack *create_stack(void)
{
	struct stack *new_stack = malloc(sizeof(struct stack));
	new_stack->number_of_nodes = 0U;
	new_stack->top = NULL;
	return new_stack;
}

int check_stack_empty(struct stack *stack)
{
	return stack->number_of_nodes == 0U;
}

void push_into_stack(struct stack *stack, char value)
{
	struct stack_node *new_node = create_stack_node(value);
	new_node->previous = stack->top;
	stack->top = new_node;
	++stack->number_of_nodes;
}

void pop_from_stack(struct stack *stack)
{
	if (stack->number_of_nodes == 0U) return;
	struct stack_node *to_remove = stack->top;
	stack->top = to_remove->previous;
	--stack->number_of_nodes;
	free(to_remove);
}

void free_stack(struct stack *stack)
{
	size_t number_of_nodes = stack->number_of_nodes;
	while (number_of_nodes--) {
		struct stack_node *to_remove = stack->top;
		stack->top = to_remove->previous;
		free(to_remove);
	}
	free(stack);
}

char peek_top_of_stack(struct stack *stack)
{
	return (stack->top) ? stack->top->character : '\0';
}
