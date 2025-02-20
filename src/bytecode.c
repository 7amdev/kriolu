#include "kriolu.h"

//
// Globals
//

Bytecode g_bytecode;

void bytecode_init(Bytecode *bytecode)
{
    instruction_array_init(&bytecode->instructions);
    value_array_init(&bytecode->values);
    line_array_init(&bytecode->lines);
}

static int bytecode_write_value(Bytecode *bytecode, Value value)
{
    return value_array_insert(&bytecode->values, value);
}

static int bytecode_write_instruction_u24(Bytecode *bytecode, uint32_t data, int line_number)
{
    // todo: assert(bytecode->instructions.items != NULL);
    int instruction_index = instruction_array_insert_u24(&bytecode->instructions, data);
    int line_index = line_array_insert_3x(&bytecode->lines, line_number);
    assert(instruction_index == line_index);

    return instruction_index;
}

int bytecode_write_byte(Bytecode *bytecode, uint8_t data, int line_number)
{
    int instruction_index = instruction_array_insert(&bytecode->instructions, data);
    int line_index = line_array_insert(&bytecode->lines, line_number);
    assert(instruction_index == line_index);

    return instruction_index;
}

int bytecode_write_constant(Bytecode *bytecode, Value value, int line_number)
{
    int value_index = bytecode_write_value(bytecode, value);
    assert(value_index > -1);

    if (value_index < 256)
    {
        bytecode_write_opcode(bytecode, OpCode_Constant, line_number);
        bytecode_write_operand_u8(bytecode, (uint8_t)value_index, line_number);
        return bytecode->instructions.count - 1;
    }

    bytecode_write_opcode(bytecode, OpCode_Constant_Long, line_number);
    bytecode_write_operand_u24(bytecode, (uint32_t)value_index, line_number);

    return bytecode->instructions.count - 1;
}

static int bytecode_debug_instruction_byte(const char *opcode_text, int ret_offset_increment)
{
    printf("%s\n", opcode_text);
    return ret_offset_increment;
}

static int bytecode_debug_instruction_2bytes(Bytecode *bytecode, const char *opcode_text, int ret_offset_increment)
{
    uint8_t value_index = bytecode->instructions.items[ret_offset_increment - 1];
    Value value = bytecode->values.items[value_index];

    printf("%-16s %4d '", opcode_text, value_index);
    printf("%g", value);
    printf("'\n");

    return ret_offset_increment;
}

static int bytecode_debug_instruction_4bytes(Bytecode *bytecode, const char *opcode_text, int ret_offset_increment)
{
    uint8_t operand_byte1 = bytecode->instructions.items[ret_offset_increment - 3];
    uint8_t operand_byte2 = bytecode->instructions.items[ret_offset_increment - 2];
    uint8_t operand_byte3 = bytecode->instructions.items[ret_offset_increment - 1];
    uint32_t value_index = (uint32_t)((((operand_byte1 << 8) | operand_byte2) << 8) | operand_byte3);
    Value value = bytecode->values.items[value_index];

    printf("%-16s %4d '", opcode_text, value_index);
    printf("%g", value);
    printf("'\n");

    return ret_offset_increment;
}

static int bytecode_debugger_print(Bytecode *bytecode, int offset)
{
    printf("%04d ", offset);
    if (offset > 0 && bytecode->lines.items[offset] == bytecode->lines.items[offset - 1])
        printf("   | ");
    else
        printf("%4d ", bytecode->lines.items[offset]);

    OpCode opcode = bytecode->instructions.items[offset];
    if (opcode == OpCode_Constant)
        return bytecode_debug_instruction_2bytes(bytecode, "OPCODE_CONSTANT", (offset + 2));
    if (opcode == OpCode_Constant_Long)
        return bytecode_debug_instruction_4bytes(bytecode, "OPCODE_CONSTANT_LONG", (offset + 4));
    if (opcode == OpCode_Addition)
        return bytecode_debug_instruction_byte("OPCODE_ADDITION", (offset + 1));
    if (opcode == OpCode_Subtraction)
        return bytecode_debug_instruction_byte("OPCODE_SUBTRACTION", (offset + 1));
    if (opcode == OpCode_Multiplication)
        return bytecode_debug_instruction_byte("OPCODE_MULTIPLICATION", (offset + 1));
    if (opcode == OpCode_Division)
        return bytecode_debug_instruction_byte("OPCODE_DIVITION", (offset + 1));
    if (opcode == OpCode_Exponentiation)
        return bytecode_debug_instruction_byte("OPCODE_EXPONENTIATION", (offset + 1));
    if (opcode == OpCode_Negation)
        return bytecode_debug_instruction_byte("OPCODE_NEGATION", (offset + 1));
    if (opcode == OpCode_Return)
        return bytecode_debug_instruction_byte("OPCODE_RETURN", (offset + 1));

    printf("Unknown OpCode %d\n", opcode);
    return (offset + 1);
}

void bytecode_disassemble(Bytecode *bytecode, const char *name)
{
    // todo: compress this code
    printf("== %s ==\n", name);
    printf("Offset  Line#  OpCode OperandIndex Operandvalue\n");

    for (int offset = 0; offset < bytecode->instructions.count;)
    {
        offset = bytecode_debugger_print(bytecode, offset);
    }
}

void bytecode_emitter_begin()
{
    bytecode_init(&g_bytecode);
}

Bytecode bytecode_emitter_end(int line_number)
{
    Bytecode ret = g_bytecode;
    bytecode_init(&g_bytecode);

    return ret;
}

void bytecode_free(Bytecode *bytecode)
{
    instruction_array_free(&bytecode->instructions);
    line_array_free(&bytecode->lines);
    value_array_free(&bytecode->values);

    instruction_array_init(&bytecode->instructions);
    line_array_init(&bytecode->lines);
    value_array_free(&bytecode->values);
}