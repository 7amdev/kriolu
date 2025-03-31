#include "kriolu.h"

void instruction_array_init(InstructionArray* instructions)
{
    instructions->items = NULL;
    instructions->count = 0;
    instructions->capacity = 0;
}

int instruction_array_insert(InstructionArray* instructions, uint8_t item)
{
    if (instructions->capacity < instructions->count + 1)
    {
        instructions->capacity = instructions->capacity < 8 ? 8 : 2 * instructions->capacity;
        instructions->items = (uint8_t*)realloc(instructions->items, instructions->capacity * sizeof(OpCode));
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

int instruction_array_insert_u24(InstructionArray* instructions, uint8_t byte1, uint8_t byte2, uint8_t byte3)
{
    instruction_array_insert(instructions, byte3);
    instruction_array_insert(instructions, byte2);
    instruction_array_insert(instructions, byte1);

    return instructions->count - 1;
}

void instruction_array_free(InstructionArray* instructions)
{
    free(instructions->items);
    instruction_array_init(instructions);
}