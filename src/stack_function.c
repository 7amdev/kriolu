#include "kriolu.h"

void StackFunction_init(StackFunction* stack_function) {
    stack_function->top = 0;
}

Function* StackFunction_push(StackFunction* stack_function, Function* function) {
    assert(!StackFunction_is_full(stack_function) && "ERROR: StackFunction is full.");

    stack_function->items[stack_function->top] = function;
    stack_function->top += 1;

    return stack_function->items[stack_function->top - 1];
}

Function* StackFunction_pop(StackFunction* stack_function) {
    assert(!StackFunction_is_empty(stack_function) && "ERROR: StackFunction is empty.");

    stack_function->top -= 1;

    return stack_function->items[stack_function->top];
}

Function* StackFunction_peek(StackFunction* stack_function, int offset) {
    int index = stack_function->top - 1 - offset;

    assert(index >= 0 && "ERROR: StackFunction index underflow.");
    assert(index < UINT8_COUNT && "ERROR: StackFunction index overflow.");

    return stack_function->items[index];
}

bool StackFunction_is_empty(StackFunction* stack_function) {
    return (stack_function->top == 0);
}

bool StackFunction_is_full(StackFunction* stack_function) {
    return (stack_function->top == UINT8_COUNT);
}

void StackFunction_free(StackFunction* stack_function) {
    free(stack_function);
}
