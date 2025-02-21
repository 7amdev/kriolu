#include "kriolu.h"

void value_stack_init(ValueStack *stack)
{
    stack->top = stack->items;
    for (int i = 0; i < STACK_MAX; i++)
        stack->items[i] = 0;
}

static int value_stack_count(ValueStack *stack)
{
    return (int)(stack->top - stack->items);
}

bool value_stack_is_full(ValueStack *stack)
{
    return (value_stack_count(stack) == STACK_MAX);
}

bool value_stack_is_empty(ValueStack *stack)
{
    return (value_stack_count(stack) == 0);
}

Value value_stack_push(ValueStack *stack, Value value)
{
    assert(value_stack_count(stack) < STACK_MAX && "Error: Stack Overflow");

    *stack->top = value;
    stack->top += 1;

    return *(stack->top - 1);
}

Value value_stack_pop(ValueStack *stack)
{
    assert(value_stack_count(stack) > 0 && "Error: Stack Underflow");

    stack->top -= 1;
    return *stack->top;
}
