#include "kriolu.h"

void stack_value_reset(StackValue *stack)
{
    stack->top = stack->items;
}

// todo: make this a macro
static int stack_value_count(StackValue *stack)
{
    return (int)(stack->top - stack->items);
}

bool stack_value_is_full(StackValue *stack)
{
    return (stack_value_count(stack) == STACK_MAX);
}

bool stack_value_is_empty(StackValue *stack)
{
    return (stack_value_count(stack) == 0);
}

Value stack_value_push(StackValue *stack, Value value)
{
    assert(stack_value_count(stack) < STACK_MAX && "Error: Stack Overflow");

    *stack->top = value;
    stack->top += 1;

    return *(stack->top - 1);
}

Value stack_value_pop(StackValue *stack)
{
    assert(stack_value_count(stack) > 0 && "Error: Stack Underflow");

    stack->top -= 1;
    return *stack->top;
}

void stack_value_trace(StackValue *stack)
{
    printf("          ");
    for (Value *slot = stack->items; slot < stack->top; slot++)
    {
        printf("[ ");
        printf("%g", *slot);
        printf(" ]");
    }
    printf("\n");
}