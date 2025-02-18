#include "kriolu.h"

void instruction_array_init(InstructionArray *instructions)
{
    instructions->items = NULL;
    instructions->count = 0;
    instructions->capacity = 0;
}

int instruction_array_insert(InstructionArray *instructions, uint8_t item)
{
    if (instructions->capacity < instructions->count + 1)
    {
        instructions->capacity = instructions->capacity < 8 ? 8 : 2 * instructions->capacity;
        instructions->items = (uint8_t *)realloc(instructions->items, instructions->capacity * sizeof(OpCode));
        assert(instructions->items);
        for (int i = instructions->capacity - 1; i >= instructions->count; --i)
        {
            instructions->items[i] = OpCode_Invalid;
        }
    }

    instructions->items[instructions->count] = item;
    instructions->count += 1;

    return instructions->count - 1;
}

int instruction_array_insert_u24(InstructionArray *instructions, uint32_t item)
{
    uint8_t _8bit1 = (uint8_t)(item & 0xFF);
    uint8_t _8bit2 = (uint8_t)((item >> 8) & 0xFF);
    uint8_t _8bit3 = (uint8_t)((item >> 16) & 0xFF);

    instruction_array_insert(instructions, _8bit3);
    instruction_array_insert(instructions, _8bit2);
    instruction_array_insert(instructions, _8bit1);

    return instructions->count - 1;
}

void instruction_array_free(InstructionArray *instructions)
{
    free(instructions->items);
    instruction_array_init(instructions);
}