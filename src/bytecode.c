#include "kriolu.h"

//
// Globals
//

Bytecode g_bytecode;

void bytecode_init(Bytecode* bytecode) {
    array_instruction_init(&bytecode->instructions);
    array_value_init(&bytecode->values);
    array_line_init(&bytecode->lines);
}

int bytecode_insert_instruction_1byte(Bytecode* bytecode, OpCode opcode, int line_number) {
    int opcode_index = array_instruction_insert(&bytecode->instructions, opcode);
    int line_opcode_index = array_line_insert(&bytecode->lines, line_number);

    assert(opcode_index == line_opcode_index);
    assert(opcode_index == (bytecode->instructions.count - 1));

    return bytecode->instructions.count - 1;
}

int bytecode_insert_instruction_2bytes(Bytecode* bytecode, OpCode opcode, uint8_t operand, int line_number) {
    int opcode_index = array_instruction_insert(&bytecode->instructions, opcode);
    int line_opcode_index = array_line_insert(&bytecode->lines, line_number);

    assert(opcode_index == line_opcode_index);

    int operand_index = array_instruction_insert(&bytecode->instructions, operand);
    int line_operand_index = array_line_insert(&bytecode->lines, line_number);

    assert(operand_index == line_operand_index);
    assert(operand_index == (bytecode->instructions.count - 1));

    return bytecode->instructions.count - 1;
}

static int bytecode_insert_instruction_4bytes(Bytecode* bytecode, OpCode opcode, uint8_t byte1, uint8_t byte2, uint8_t byte3, int line_number) {
    int opcode_index = array_instruction_insert(&bytecode->instructions, opcode);
    int line_opcode_index = array_line_insert(&bytecode->lines, line_number);

    assert(opcode_index == line_opcode_index);

    int operand_index = array_instruction_insert_u24(&bytecode->instructions, byte1, byte2, byte3);
    int line_index = array_line_insert_3x(&bytecode->lines, line_number);

    assert(operand_index == line_index);
    assert(operand_index == (bytecode->instructions.count - 1));

    return bytecode->instructions.count - 1;
}

int bytecode_insert_instruction_constant(Bytecode* bytecode, Value value, int line_number) {
    int value_index = array_value_insert(&bytecode->values, value);
    assert(value_index > -1);

    if (value_index < 256) {
        return bytecode_insert_instruction_2bytes(
            bytecode,
            OpCode_Constant,       // OpCode
            (uint8_t)value_index,  // Operand
            line_number
        );
    }

    uint8_t byte1 = (value_index >> 0 & 0xff);
    uint8_t byte2 = (value_index >> 8 & 0xff);
    uint8_t byte3 = (value_index >> 16 & 0xff);

    return bytecode_insert_instruction_4bytes(
        bytecode,
        OpCode_Constant,          // OpCode
        byte1, byte2, byte3,      // Operand
        line_number
    );
}

static int bytecode_debug_instruction_byte(const char* opcode_text, int ret_offset_increment)
{
    printf("%s\n", opcode_text);
    return ret_offset_increment;
}

static int bytecode_debug_instruction_2bytes(Bytecode* bytecode, const char* opcode_text, int ret_offset_increment)
{
    uint8_t operand = bytecode->instructions.items[ret_offset_increment - 1];
    Value value = bytecode->values.items[operand];

    if (strcmp(opcode_text, "OPCODE_INTERPOLATION") == 0)
    {
        printf("%-22s %5s '%d'\n", opcode_text, "**", operand);
    } else
    {
        printf("%-22s %5d '", opcode_text, operand);
        value_print(value);
        printf("'\n");
    }

    return ret_offset_increment;
}

static int bytecode_debug_instruction_4bytes(Bytecode* bytecode, const char* opcode_text, int ret_offset_increment)
{
    uint8_t operand_byte1 = bytecode->instructions.items[ret_offset_increment - 3];
    uint8_t operand_byte2 = bytecode->instructions.items[ret_offset_increment - 2];
    uint8_t operand_byte3 = bytecode->instructions.items[ret_offset_increment - 1];
    uint32_t value_index = (uint32_t)((((operand_byte1 << 8) | operand_byte2) << 8) | operand_byte3);
    Value value = bytecode->values.items[value_index];

    printf("%-16s %4d '", opcode_text, value_index);
    value_print(value); // printf("%g", value);
    printf("'\n");

    return ret_offset_increment;
}

int bytecode_disassemble_instruction(Bytecode* bytecode, int offset)
{
    printf("%06d ", offset);
    if (offset > 0 && bytecode->lines.items[offset] == bytecode->lines.items[offset - 1])
        printf("   | ");
    else
        printf("%4d ", bytecode->lines.items[offset]);

    OpCode opcode = bytecode->instructions.items[offset];
    if (opcode == OpCode_Constant)
        return bytecode_debug_instruction_2bytes(bytecode, "OPCODE_CONSTANT", (offset + 2));
    if (opcode == OpCode_Interpolation)
        return bytecode_debug_instruction_2bytes(bytecode, "OPCODE_INTERPOLATION", (offset + 2));
    if (opcode == OpCode_Constant_Long)
        return bytecode_debug_instruction_4bytes(bytecode, "OPCODE_CONSTANT_LONG", (offset + 4));
    if (opcode == OpCode_True)
        return bytecode_debug_instruction_byte("OPCODE_TRUE", (offset + 1));
    if (opcode == OpCode_False)
        return bytecode_debug_instruction_byte("OPCODE_FALSE", (offset + 1));
    if (opcode == OpCode_Pop)
        return bytecode_debug_instruction_byte("OPCODE_POP", (offset + 1));
    if (opcode == OpCode_Define_Global)
        return bytecode_debug_instruction_2bytes(bytecode, "OPCODE_DEFINE_GLOBAL", (offset + 2));
    if (opcode == OpCode_Read_Global)
        return bytecode_debug_instruction_2bytes(bytecode, "OPCODE_READ_GLOBAL", (offset + 2));
    if (opcode == OpCode_Assign_Global)
        return bytecode_debug_instruction_2bytes(bytecode, "OPCODE_ASSIGN_GLOBAL", (offset + 2));
    if (opcode == OpCode_Nil)
        return bytecode_debug_instruction_byte("OPCODE_NIL", (offset + 1));
    if (opcode == OpCode_Negation)
        return bytecode_debug_instruction_byte("OPCODE_NEGATION", (offset + 1));
    if (opcode == OpCode_Not)
        return bytecode_debug_instruction_byte("OPCODE_NOT", (offset + 1));
    if (opcode == OpCode_Addition)
        return bytecode_debug_instruction_byte("OPCODE_ADDITION", (offset + 1));
    if (opcode == OpCode_Subtraction)
        return bytecode_debug_instruction_byte("OPCODE_SUBTRACTION", (offset + 1));
    if (opcode == OpCode_Multiplication)
        return bytecode_debug_instruction_byte("OPCODE_MULTIPLICATION", (offset + 1));
    if (opcode == OpCode_Division)
        return bytecode_debug_instruction_byte("OPCODE_DIVISION", (offset + 1));
    if (opcode == OpCode_Exponentiation)
        return bytecode_debug_instruction_byte("OPCODE_EXPONENTIATION", (offset + 1));
    if (opcode == OpCode_Negation)
        return bytecode_debug_instruction_byte("OPCODE_NEGATION", (offset + 1));
    if (opcode == OpCode_Equal_To)
        return bytecode_debug_instruction_byte("OPCODE_EQUAL_TO", (offset + 1));
    if (opcode == OpCode_Greater_Than)
        return bytecode_debug_instruction_byte("OPCODE_GREATER_THAN", (offset + 1));
    if (opcode == OpCode_Less_Than)
        return bytecode_debug_instruction_byte("OPCODE_LESS_THAN", (offset + 1));
    if (opcode == OpCode_Print)
        return bytecode_debug_instruction_byte("OPCODE_PRINT", (offset + 1));
    if (opcode == OpCode_Return)
        return bytecode_debug_instruction_byte("OPCODE_RETURN", (offset + 1));

    assert(false && "Unsupported OpCode. Handle the OpCode by adding a if statement.");
    printf("Unknown OpCode %d\n", opcode);
    return (offset + 1);
}

void bytecode_disassemble(Bytecode* bytecode, const char* name)
{
    printf("===== %s =====\n", name);
    printf("                                     Operand  \n");
    printf("Offset Line         OpCode         index value\n");
    printf("------ ---- ---------------------- ----- -----\n");

    for (int offset = 0; offset < bytecode->instructions.count;)
    {
        offset = bytecode_disassemble_instruction(bytecode, offset);
    }
}

void bytecode_emitter_begin()
{
    bytecode_init(&g_bytecode);
}

Bytecode bytecode_emitter_end(int line_number)
{
    Bytecode ret = g_bytecode; // copy
    bytecode_init(&g_bytecode); // reset global variable

    return ret; // return copy
}

void bytecode_free(Bytecode* bytecode)
{
    array_instruction_free(&bytecode->instructions);
    array_line_free(&bytecode->lines);
    array_value_free(&bytecode->values);

    array_instruction_init(&bytecode->instructions);
    array_line_init(&bytecode->lines);
    array_value_init(&bytecode->values);
}