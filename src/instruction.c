#include "kriolu.h"

void array_instruction_init(ArrayInstruction* instructions) {
    instructions->items = NULL;
    instructions->count = 0;
    instructions->capacity = 0;
}

int array_instruction_insert(ArrayInstruction* instructions, uint8_t item) {
    if (instructions->capacity < instructions->count + 1) {
        int old_capacity = instructions->count;
        instructions->capacity = instructions->capacity < 8 ? 8 : 2 * instructions->capacity;
        instructions->items = Memory_AllocateArray(uint8_t, instructions->items, old_capacity, instructions->capacity);
        assert(instructions->items);
        for (int i = instructions->capacity - 1; i >= instructions->count; --i) {
            instructions->items[i] = OpCode_Invalid;
        }
    }

    instructions->items[instructions->count] = item;
    instructions->count += 1;

    return instructions->count - 1;
}

int array_instruction_insert_u24(ArrayInstruction* instructions, uint8_t byte1, uint8_t byte2, uint8_t byte3) {
    array_instruction_insert(instructions, byte3);
    array_instruction_insert(instructions, byte2);
    array_instruction_insert(instructions, byte1);

    return instructions->count - 1;
}

void array_instruction_free(ArrayInstruction* instructions) {
    free(instructions->items);
    array_instruction_init(instructions);
}