#include "kriolu.h"

void StackBlock_init(StackBlock* blocks) {
    blocks->top = 0;
}

BlockType StackBlock_push(StackBlock* blocks, BlockType value) {
    assert(blocks->top < BREAK_POINT_MAX && "Error: StackBlock overflow!");

    blocks->items[blocks->top] = value;
    blocks->top += 1;

    return blocks->items[blocks->top - 1];
}

BlockType StackBlock_pop(StackBlock* blocks) {
    assert(blocks->top > 0 && "Error: StackBlock underflow!");

    blocks->top -= 1;

    return blocks->items[blocks->top];
}

BlockType StackBlock_peek(StackBlock* blocks, int offset) {
    int index = blocks->top - 1 - offset;
    assert(index >= 0 && index < BREAK_POINT_MAX);

    return blocks->items[index];
}

bool StackBlock_is_empty(StackBlock* blocks) {
    return (blocks->top == 0);
}

bool StackBlock_is_full(StackBlock* blocks) {
    return (blocks->top == BLOCKS_MAX);
}

int StackBlock_get_top_item_index(StackBlock* blocks) {
    return blocks->top - 1;
}