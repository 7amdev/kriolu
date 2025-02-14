#include "kriolu.h"

void instruction_array_init(InstructionArray *instructions)
{
    instructions->items = NULL;
    instructions->lines = NULL;
    instructions->count = 0;
    instructions->capacity = 0;
}

uint32_t instruction_array_insert(InstructionArray *instructions, uint8_t item, int line_number)
{
    if (instructions->capacity < instructions->count + 1)
    {
        instructions->capacity = instructions->capacity < 8 ? 8 : 2 * instructions->capacity;
        instructions->items = (uint8_t *)realloc(instructions->items, instructions->capacity * sizeof(OpCode));
        instructions->lines = (int *)realloc(instructions->lines, instructions->capacity * sizeof(int));
        assert(instructions->items);
        assert(instructions->lines);
        for (
            int i = instructions->capacity - 1;
            // cast is necessary, because (int(i) = -1) >= (uint32_t(count) = 0) is true
            i >= (int)(instructions->count);
            --i)
        {
            instructions->items[i] = OpCode_Invalid;
        }
    }

    instructions->items[instructions->count] = item;
    instructions->lines[instructions->count] = line_number;
    instructions->count += 1;

    return instructions->count - 1;
}

uint32_t instruction_array_insert_opcode(InstructionArray *instructions, OpCode opcode, int line_number)
{
    return instruction_array_insert(instructions, opcode, line_number);
}

uint32_t instruction_array_insert_operand_u8(InstructionArray *instructions, uint8_t operand, int line_number)
{
    return instruction_array_insert(instructions, operand, line_number);
}

// TODO
uint32_t instruction_array_insert_operand_u24(InstructionArray *instructions, uint32_t operand, int line_number)
{
    uint8_t _8bit1 = (uint8_t)(operand & 0x00FF);
    uint8_t _8bit2 = (uint8_t)((operand >> 8) & 0xFF);
    uint8_t _8bit3 = (uint8_t)((operand >> 16) & 0xFF);

    instruction_array_insert(instructions, _8bit3, line_number);
    instruction_array_insert(instructions, _8bit2, line_number);
    instruction_array_insert(instructions, _8bit1, line_number);

    // uint32_t _operand = (uint32_t)((((_8bit3 << 8) | _8bit2) << 8) | _8bit1);

    return instructions->count - 1;
}

void instruction_array_free(InstructionArray *instructions)
{
    free(instructions->items);
    free(instructions->lines);
    instruction_array_init(instructions);
}