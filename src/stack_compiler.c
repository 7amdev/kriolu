#include "kriolu.h"

void StackCompiler_init(StackCompiler* stack_compiler) {
    stack_compiler->top = 0;
}

Compiler* StackCompiler_push(StackCompiler* stack_compiler, Compiler* compiler) {
    assert(!StackCompiler_is_full(stack_compiler) && "ERROR: StackCompiler is full.");

    stack_compiler->items[stack_compiler->top] = compiler;
    stack_compiler->top += 1;

    return stack_compiler->items[stack_compiler->top - 1];
}

Compiler* StackCompiler_pop(StackCompiler* stack_compiler) {
    assert(!StackCompiler_is_empty(stack_compiler) && "ERROR: StackCompiler is empty.");

    stack_compiler->top -= 1;

    return stack_compiler->items[stack_compiler->top];
}

Compiler* StackCompiler_peek(StackCompiler* stack_compiler, int offset) {
    int index = stack_compiler->top - 1 - offset;

    assert(index >= 0 && "ERROR: StackCompiler index underflow.");
    assert(index < UINT8_COUNT && "ERROR: StackCompiler index overflow.");

    return stack_compiler->items[index];
}

bool StackCompiler_is_empty(StackCompiler* stack_compiler) {
    return (stack_compiler->top == 0);
}

bool StackCompiler_is_full(StackCompiler* stack_compiler) {
    return (stack_compiler->top == UINT8_COUNT);
}

void StackCompiler_free(StackCompiler* stack_compiler) {
    free(stack_compiler);
}
