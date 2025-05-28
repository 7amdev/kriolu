#include "kriolu.h"

void StackBreak_init(StackBreakpoint* breakpoints) {
    breakpoints->top = 0;
}

Breakpoint StackBreak_push(StackBreakpoint* breakpoints, Breakpoint value) {
    assert(breakpoints->top < BREAK_POINT_MAX && "Error: StackBreak overflow!");

    breakpoints->items[breakpoints->top] = value;
    breakpoints->top += 1;

    return breakpoints->items[breakpoints->top - 1];
}

Breakpoint StackBreak_pop(StackBreakpoint* breakpoints) {
    assert(breakpoints->top > 0 && "Error: StackBreak underflow!");

    breakpoints->top -= 1;

    return breakpoints->items[breakpoints->top];
}

Breakpoint StackBreak_peek(StackBreakpoint* breakpoints, int offset) {
    return breakpoints->items[breakpoints->top - 1 - offset];
}

bool StackBreak_is_empty(StackBreakpoint* breakpoints) {
    return breakpoints->top == 0;
}

bool StackBreak_is_full(StackBreakpoint* breakpoints) {
    return breakpoints->top == BREAK_POINT_MAX;
}
