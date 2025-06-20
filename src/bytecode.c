#include "kriolu.h"

//
// Globals
//

// Bytecode g_bytecode;

void Bytecode_init(Bytecode* bytecode) {
    array_instruction_init(&bytecode->instructions);
    array_value_init(&bytecode->values);
    array_line_init(&bytecode->lines);
}

int Bytecode_insert_instruction_1byte(Bytecode* bytecode, OpCode opcode, int line_number) {
    int opcode_index = array_instruction_insert(&bytecode->instructions, opcode);
    int line_opcode_index = array_line_insert(&bytecode->lines, line_number);

    assert(opcode_index == line_opcode_index);
    assert(opcode_index == (bytecode->instructions.count - 1));

    return bytecode->instructions.count - 1;
}

int Bytecode_insert_instruction_2bytes(Bytecode* bytecode, OpCode opcode, uint8_t operand, int line_number) {
    int opcode_index = array_instruction_insert(&bytecode->instructions, opcode);
    int line_opcode_index = array_line_insert(&bytecode->lines, line_number);

    assert(opcode_index == line_opcode_index);

    int operand_index = array_instruction_insert(&bytecode->instructions, operand);
    int line_operand_index = array_line_insert(&bytecode->lines, line_number);

    assert(operand_index == line_operand_index);
    assert(operand_index == (bytecode->instructions.count - 1));

    return bytecode->instructions.count - 1;
}

static int Bytecode_insert_instruction_4bytes(Bytecode* bytecode, OpCode opcode, uint8_t byte1, uint8_t byte2, uint8_t byte3, int line_number) {
    int opcode_index = array_instruction_insert(&bytecode->instructions, opcode);
    int line_opcode_index = array_line_insert(&bytecode->lines, line_number);

    assert(opcode_index == line_opcode_index);

    int operand_index = array_instruction_insert_u24(&bytecode->instructions, byte1, byte2, byte3);
    int line_index = array_line_insert_3x(&bytecode->lines, line_number);

    assert(operand_index == line_index);
    assert(operand_index == (bytecode->instructions.count - 1));

    return bytecode->instructions.count - 1;
}

int Bytecode_insert_instruction_constant(Bytecode* bytecode, Value value, int line_number) {
    int value_index = array_value_insert(&bytecode->values, value);
    assert(value_index > -1);

    if (value_index < 256) {
        return Bytecode_insert_instruction_2bytes(
            bytecode,
            OpCode_Constant,       // OpCode
            (uint8_t)value_index,  // Operand
            line_number
        );
    }

    uint8_t byte1 = (value_index >> 0 & 0xff);
    uint8_t byte2 = (value_index >> 8 & 0xff);
    uint8_t byte3 = (value_index >> 16 & 0xff);

    return Bytecode_insert_instruction_4bytes(
        bytecode,
        OpCode_Constant_Long,     // OpCode
        byte1, byte2, byte3,      // Operand
        line_number
    );
}

// Jumps Forward
//
int Bytecode_insert_instruction_jump(Bytecode* bytecode, OpCode opcode, int line) {
    Bytecode_insert_instruction_1byte(bytecode, opcode, line);
    Bytecode_insert_instruction_1byte(bytecode, 0xff, line);
    Bytecode_insert_instruction_1byte(bytecode, 0xff, line);

    return bytecode->instructions.count - 2;
}

// Jumps Backwards
//
void Bytecode_emit_instruction_loop(Bytecode* bytecode, int loop_start_index, int line_number) {
    // instruction_array_current_position + loop_instruction_size(3 bytes) - increment_index_in_instruction_array
    //
    int offset = bytecode->instructions.count + 3 - loop_start_index;
    uint8_t operand_byte1 = ((offset >> 8) & 0xff);
    uint8_t operand_byte2 = (offset & 0xff);

    Bytecode_insert_instruction_1byte(bytecode, OpCode_Loop, line_number);
    Bytecode_insert_instruction_1byte(bytecode, operand_byte1, line_number);
    Bytecode_insert_instruction_1byte(bytecode, operand_byte2, line_number);
}

bool Bytecode_patch_instruction_jump(Bytecode* bytecode, int operand_index) {
    int jump_to_index = bytecode->instructions.count - operand_index - 2;
    if (jump_to_index > UINT16_MAX) return true;

    bytecode->instructions.items[operand_index] = (jump_to_index >> 8) & 0xff;
    bytecode->instructions.items[operand_index + 1] = jump_to_index & 0xff;

    return false;
}

static int Bytecode_debug_instruction_byte(const char* opcode_text, int ret_offset_increment)
{
    printf("%s\n", opcode_text);
    return ret_offset_increment;
}

static int Bytecode_debug_instruction_2bytes(Bytecode* bytecode, const char* opcode_text, int ret_offset_increment)
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

static int Bytecode_debug_instruction_local(Bytecode* bytecode, const char* opcode_text, int ret_offset_increment)
{
    uint8_t operand = bytecode->instructions.items[ret_offset_increment - 1];

    printf("%-22s %5d '", opcode_text, operand);
    printf("'\n");
    return ret_offset_increment;
}

// TODO: rename to bytecode_debug_instruction_jump(bytecode, text, sign, offset);
//
static int Bytecode_debug_instruction_3bytes(Bytecode* bytecode, const char* opcode_text, int ret_offset_increment) {
    uint8_t operand_byte1 = bytecode->instructions.items[ret_offset_increment - 2];
    uint8_t operand_byte2 = bytecode->instructions.items[ret_offset_increment - 1];
    uint16_t operand_value = (uint16_t)((operand_byte1 << 8) | operand_byte2);

    printf("%-22s %5d '", opcode_text, operand_value);
    printf("'\n");

    return ret_offset_increment;
}

static int Bytecode_debug_instruction_jump(Bytecode* bytecode, const char* text, int sign, int ret_offset_increment) {
    uint8_t operand_byte1 = bytecode->instructions.items[ret_offset_increment - 2];
    uint8_t operand_byte2 = bytecode->instructions.items[ret_offset_increment - 1];
    uint16_t operand_value = (uint16_t)((operand_byte1 << 8) | operand_byte2);
    int current_offset = ret_offset_increment - 3;

    printf("%-22s %5d -> %d\n", text, current_offset, ret_offset_increment + operand_value * sign);

    return ret_offset_increment;
}

static int Bytecode_debug_instruction_4bytes(Bytecode* bytecode, const char* opcode_text, int ret_offset_increment)
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

int Bytecode_disassemble_instruction(Bytecode* bytecode, int offset)
{
    // TODO:                                 Operand
    // Offset    Line         OpCode         index value
    // --------- ---- ---------------------- ----- -----
    // 000000 +1    1 OPCODE_FALSE
    // 000001 +3    | OPCODE_JUMP_IF_FALSE       7 ''
    // 000004 +1    | OPCODE_POP
    // 000005 +2    2 OPCODE_CONSTANT            0 'verdadi'
    // 000007 +1    | OPCODE_PRINT

    printf("%06d ", offset);
    if (offset > 0 && bytecode->lines.items[offset] == bytecode->lines.items[offset - 1])
        printf("   | ");
    else
        printf("%4d ", bytecode->lines.items[offset]);

    OpCode opcode = bytecode->instructions.items[offset];
    if (opcode == OpCode_Constant)
        return Bytecode_debug_instruction_2bytes(bytecode, "OPCODE_CONSTANT", (offset + 2));
    if (opcode == OpCode_Interpolation)
        return Bytecode_debug_instruction_2bytes(bytecode, "OPCODE_INTERPOLATION", (offset + 2));
    if (opcode == OpCode_Constant_Long)
        return Bytecode_debug_instruction_4bytes(bytecode, "OPCODE_CONSTANT_LONG", (offset + 4));
    if (opcode == OpCode_True)
        return Bytecode_debug_instruction_byte("OPCODE_TRUE", (offset + 1));
    if (opcode == OpCode_False)
        return Bytecode_debug_instruction_byte("OPCODE_FALSE", (offset + 1));
    if (opcode == OpCode_Pop)
        return Bytecode_debug_instruction_byte("OPCODE_POP", (offset + 1));
    if (opcode == OpCode_Define_Global)
        return Bytecode_debug_instruction_2bytes(bytecode, "OPCODE_DEFINE_GLOBAL", (offset + 2));
    if (opcode == OpCode_Read_Global)
        return Bytecode_debug_instruction_2bytes(bytecode, "OPCODE_READ_GLOBAL", (offset + 2));
    if (opcode == OpCode_Assign_Global)
        return Bytecode_debug_instruction_2bytes(bytecode, "OPCODE_ASSIGN_GLOBAL", (offset + 2));
    if (opcode == OpCode_Read_Local)
        return Bytecode_debug_instruction_local(bytecode, "OPCODE_READ_LOCAL", (offset + 2));
    if (opcode == OpCode_Assign_Local)
        return Bytecode_debug_instruction_local(bytecode, "OPCODE_ASSIGN_LOCAL", (offset + 2));
    if (opcode == OpCode_Function_Call)
        return Bytecode_debug_instruction_2bytes(bytecode, "OPCODE_FUNCTION_CALL", (offset + 2));
    if (opcode == OpCode_Nil)
        return Bytecode_debug_instruction_byte("OPCODE_NIL", (offset + 1));
    if (opcode == OpCode_Negation)
        return Bytecode_debug_instruction_byte("OPCODE_NEGATION", (offset + 1));
    if (opcode == OpCode_Not)
        return Bytecode_debug_instruction_byte("OPCODE_NOT", (offset + 1));
    if (opcode == OpCode_Addition)
        return Bytecode_debug_instruction_byte("OPCODE_ADDITION", (offset + 1));
    if (opcode == OpCode_Subtraction)
        return Bytecode_debug_instruction_byte("OPCODE_SUBTRACTION", (offset + 1));
    if (opcode == OpCode_Multiplication)
        return Bytecode_debug_instruction_byte("OPCODE_MULTIPLICATION", (offset + 1));
    if (opcode == OpCode_Division)
        return Bytecode_debug_instruction_byte("OPCODE_DIVISION", (offset + 1));
    if (opcode == OpCode_Exponentiation)
        return Bytecode_debug_instruction_byte("OPCODE_EXPONENTIATION", (offset + 1));
    if (opcode == OpCode_Negation)
        return Bytecode_debug_instruction_byte("OPCODE_NEGATION", (offset + 1));
    if (opcode == OpCode_Equal_To)
        return Bytecode_debug_instruction_byte("OPCODE_EQUAL_TO", (offset + 1));
    if (opcode == OpCode_Greater_Than)
        return Bytecode_debug_instruction_byte("OPCODE_GREATER_THAN", (offset + 1));
    if (opcode == OpCode_Less_Than)
        return Bytecode_debug_instruction_byte("OPCODE_LESS_THAN", (offset + 1));
    if (opcode == OpCode_Print)
        return Bytecode_debug_instruction_byte("OPCODE_PRINT", (offset + 1));
    if (opcode == OpCode_Jump_If_False)
        return Bytecode_debug_instruction_jump(bytecode, "OPCODE_JUMP_IF_FALSE", 1, (offset + 3));
    if (opcode == OpCode_Jump)
        return Bytecode_debug_instruction_jump(bytecode, "OPCODE_JUMP", 1, (offset + 3));
    if (opcode == OpCode_Loop)
        return Bytecode_debug_instruction_jump(bytecode, "OPCODE_LOOP", -1, (offset + 3));
    if (opcode == OpCode_Return)
        return Bytecode_debug_instruction_byte("OPCODE_RETURN", (offset + 1));

    assert(false && "Unsupported OpCode. Handle the OpCode by adding a if statement.");
    printf("Unknown OpCode %d\n", opcode);
    return (offset + 1);
}

void Bytecode_disassemble_header(char* title_name) {
    int title_length = strlen(title_name) + 2;
    int title_width = 46;
    int title_padding = (46 - title_length) / 2;

    for (int i = 0; i < title_padding; i++) printf("=");
    printf(" %s ", title_name == NULL ? "script" : title_name);
    for (int i = 0; i < title_padding; i++) printf("=");
    printf("\n");
    printf("                                     Operand  \n");
    printf("Offset Line         OpCode         index value\n");
    printf("------ ---- ---------------------- ----- -----\n");
}

void Bytecode_disassemble(Bytecode* bytecode, const char* name) {
    Bytecode_disassemble_header(name);
    for (int offset = 0; offset < bytecode->instructions.count;) {
        offset = Bytecode_disassemble_instruction(bytecode, offset);
    }
}

// void Bytecode_emitter_begin()
// {
//     Bytecode_init(&g_bytecode);
// }

// Bytecode Bytecode_emitter_end(int line_number)
// {
//     Bytecode ret = g_bytecode; // copy
//     Bytecode_init(&g_bytecode); // reset global variable

//     return ret; // return copy
// }

void Bytecode_free(Bytecode* bytecode)
{
    array_instruction_free(&bytecode->instructions);
    array_line_free(&bytecode->lines);
    array_value_free(&bytecode->values);

    array_instruction_init(&bytecode->instructions);
    array_line_init(&bytecode->lines);
    array_value_init(&bytecode->values);
}