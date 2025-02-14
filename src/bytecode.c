#include "kriolu.h"

void bytecode_init(Bytecode *bytecode)
{
    instruction_array_init(&bytecode->instructions);
    value_array_init(&bytecode->values);
}

// todo: rename to bytecode_instruction_write_opcode
void bytecode_write_instruction(Bytecode *bytecode, OpCode instruction_code, int line_number)
{
    instruction_array_insert(&bytecode->instructions, instruction_code, line_number);
}

uint32_t bytecode_write_value(Bytecode *bytecode, Value value)
{
    return value_array_insert(&bytecode->values, value);
}

// todo: rename to bytecode_instruction_write_constant
void bytecode_write_constant(Bytecode *bytecode, Value value, int line_number)
{
    uint32_t value_index = bytecode_write_value(bytecode, value);
    if (value_index < 256)
    {
        instruction_array_insert_opcode(&bytecode->instructions, OpCode_Constant, line_number);
        instruction_array_insert_operand_u8(&bytecode->instructions, value_index, line_number);
        return;
    }

    instruction_array_insert_opcode(&bytecode->instructions, OpCode_Constant_Long, line_number);
    instruction_array_insert_operand_u24(&bytecode->instructions, value_index, line_number);

    // bytecode_write_instruction(bytecode, (uint8_t)OpCode_Constant, 123);
    // bytecode_write_instruction(bytecode, (uint8_t)value_index, 123);
}

void bytecode_disassemble(Bytecode *bytecode, const char *name)
{
    printf("== %s ==\n", name);
    printf("Offset  Line#  OpCode OperandIndex Operandvalue\n");

    int offset = 0;
    while (offset < bytecode->instructions.count)
    {
        printf("%04d ", offset);
        if (offset > 0 && bytecode->instructions.lines[offset] == bytecode->instructions.lines[offset - 1])
        {
            printf("   | ");
        }
        else
        {
            printf("%4d ", bytecode->instructions.lines[offset]);
        }

        OpCode opcode = bytecode->instructions.items[offset];
        if (opcode == OpCode_Invalid)
        {
            printf("Invalid OpCode %d\n", opcode);

            offset += 1;
            continue;
        }
        else if (opcode == OpCode_Constant)
        {
            uint8_t value_index = bytecode->instructions.items[offset + 1];
            Value value = bytecode->values.items[value_index];
            printf("%-16s %4d '", "OPCODE_CONSTANT", value_index);
            printf("%g", value);
            printf("'\n");

            offset += 2;
            continue;
        }
        else if (opcode == OpCode_Constant_Long)
        {
            uint8_t operand_byte1 = bytecode->instructions.items[offset + 1];
            uint8_t operand_byte2 = bytecode->instructions.items[offset + 2];
            uint8_t operand_byte3 = bytecode->instructions.items[offset + 3];
            uint32_t value_index = (uint32_t)((((operand_byte1 << 8) | operand_byte2) << 8) | operand_byte3);
            Value value = bytecode->values.items[value_index];

            printf("%-16s %4d '", "OPCODE_CONSTANT_LONG", value_index);
            printf("%g", value);
            printf("'\n");

            offset += 4;
            continue;
        }
        else if (opcode == OpCode_Return)
        {
            printf("OPCODE_RETURN\n");

            offset += 1;
            continue;
        }

        printf("Unknown OpCode %d\n", opcode);
        offset += 1;

        // offset = instruction_disassemble(&bytecode->instructions, offset);
    }
}

void bytecode_free(Bytecode *bytecode)
{
    instruction_array_free(&bytecode->instructions);
    value_array_free(&bytecode->values);
}