#include "kriolu.h"

void StackFunctionCall_reset(StackFunctionCall* function_calls) {
    function_calls->top = 0;
}

FunctionCall* StackFunctionCall_push(StackFunctionCall* function_calls, ObjectFunction* function, uint8_t* ip, Value* frame_start) {
    assert(function_calls->top < FRAME_STACK_MAX);

    FunctionCall* new_function_call = &function_calls->items[function_calls->top];

    new_function_call->function = function;
    new_function_call->frame_start = frame_start;
    new_function_call->ip = ip;

    function_calls->top += 1;
    return &function_calls->items[function_calls->top - 1];
}

FunctionCall* StackFunctionCall_pop(StackFunctionCall* function_calls) {
    assert(function_calls->top > 0);

    FunctionCall* function_call = &function_calls->items[function_calls->top - 1];
    function_calls->top -= 1;

    return function_call;
}

FunctionCall* StackFunctionCall_peek(StackFunctionCall* function_calls, int offset) {
    return &function_calls->items[function_calls->top - 1 - offset];
}

bool StackFunctionCall_is_empty(StackFunctionCall* function_calls) {
    return (function_calls->top == 0);
}

bool StackFunctionCall_is_full(StackFunctionCall* function_calls) {
    return (function_calls->top == FRAME_STACK_MAX);
}