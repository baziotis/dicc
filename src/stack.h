#ifndef __STACK_HEADER__
#define __STACK_HEADER__

typedef struct stack stack_t;

stack_t *create_stack(void);
int check_stack_empty(stack_t *stack);
char peek_top_of_stack(stack_t *stack);
void push_into_stack(stack_t *stack, char value);
void pop_from_stack(stack_t *stack);
void free_stack(stack_t *stack);

#endif
