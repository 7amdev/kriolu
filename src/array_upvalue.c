#include "kriolu.h"

static int ArrayUpvalue_exists(ArrayUpvalue* upvalues, uint8_t index, bool is_local);

void ArrayUpvalue_init(ArrayUpvalue* upvalues, int count) {
    upvalues->count = count;
}

int ArrayUpvalue_add(ArrayUpvalue* upvalues, uint8_t index, bool is_local, int* function_upvalue_count) {
    assert(upvalues->count < UINT8_COUNT && "You've reached the limit of Upvalues.");

    int upvalue_index = ArrayUpvalue_find_index(upvalues, index, is_local);
    if (upvalue_index != -1) return upvalue_index;

    if (upvalues->count == UINT8_COUNT) return 0; /// Maybe -2??

    Upvalue* new_upvalue = &upvalues->items[upvalues->count];
    new_upvalue->index = index;
    new_upvalue->is_local = is_local;
    upvalues->count += 1;

    // Update ObjectFunction field 'function_upvalue_count'
    //
    if (function_upvalue_count != NULL)
        *function_upvalue_count = upvalues->count;

    return upvalues->count - 1;
}

static int ArrayUpvalue_find_index(ArrayUpvalue* upvalues, uint8_t index, bool is_local) {
    for (int i = 0; i < upvalues->count; i++) {
        if (upvalues->items[i].index == index && upvalues->items[i].is_local == is_local)
            return i;
    }

    return -1;
}