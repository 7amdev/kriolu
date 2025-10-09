#include "kriolu.h"

//
// Globals
//

// Bytecode g_bytecode;

void Bytecode_init(Bytecode* bytecode) {
    array_instruction_init(&bytecode->instructions);
    ArrayValue_init(&bytecode->values);
    array_line_init(&bytecode->lines);
}

int Bytecode_insert_instruction_1byte(Bytecode* bytecode, OpCode opcode, int line_number, bool debug_trace_on) {
    int opcode_index = array_instruction_insert(&bytecode->instructions, opcode);
    int line_opcode_index = array_line_insert(&bytecode->lines, line_number);

    assert(opcode_index == line_opcode_index);
    assert(opcode_index == (bytecode->instructions.count - 1));

    if (debug_trace_on) Bytecode_disassemble_instruction(bytecode, opcode_index);

    return bytecode->instructions.count - 1;
}

int Bytecode_insert_instruction_2bytes(Bytecode* bytecode, OpCode opcode, uint8_t operand, int line_number, bool debug_trace_on) {
    int opcode_index      = array_instruction_insert(&bytecode->instructions, opcode);
    int line_opcode_index = array_line_insert(&bytecode->lines, line_number);

    assert(opcode_index == line_opcode_index);

    int operand_index      = array_instruction_insert(&bytecode->instructions, operand);
    int line_operand_index = array_line_insert(&bytecode->lines, line_number);

    assert(operand_index == line_operand_index);
    assert(operand_index == (bytecode->instructions.count - 1));

    if (debug_trace_on) Bytecode_disassemble_instruction(bytecode, opcode_index);

    return bytecode->instructions.count - 1;
}

static int Bytecode_insert_instruction_4bytes(Bytecode* bytecode, OpCode opcode, uint8_t byte1, uint8_t byte2, uint8_t byte3, int line_number, bool debug_trace_on) {
    int opcode_index = array_instruction_insert(&bytecode->instructions, opcode);
    int line_opcode_index = array_line_insert(&bytecode->lines, line_number);

    assert(opcode_index == line_opcode_index);

    int operand_index = array_instruction_insert_u24(&bytecode->instructions, byte1, byte2, byte3);
    int line_index = array_line_insert_3x(&bytecode->lines, line_number);

    assert(operand_index == line_index);
    assert(operand_index == (bytecode->instructions.count - 1));

    if (debug_trace_on) Bytecode_disassemble_instruction(bytecode, opcode_index);

    return bytecode->instructions.count - 1;
}

int Bytecode_insert_instruction_constant(Bytecode* bytecode, Value value, int line_number, bool debug_trace_on) {
    int value_index = ArrayValue_insert(&bytecode->values, value);
    assert(value_index > -1);

    if (value_index < 256) {
        return Bytecode_insert_instruction_2bytes(
            bytecode,
            OpCode_Stack_Push_Literal,      // OpCode
            (uint8_t)value_index,           // Operand
            line_number,
            DEBUG_TRACE_INSTRUCTION
        );
    }

    uint8_t byte1 = (value_index >> 0 & 0xff);
    uint8_t byte2 = (value_index >> 8 & 0xff);
    uint8_t byte3 = (value_index >> 16 & 0xff);

    return Bytecode_insert_instruction_4bytes(
        bytecode,
        OpCode_Stack_Push_Literal_Long,     // OpCode
        byte1, byte2, byte3,                // Operand
        line_number,
        DEBUG_TRACE_INSTRUCTION
    );
}

void Bytecode_insert_instruction_closure(Bytecode* bytecode, Value value, int line_number, bool debug_trace_on) {
    int value_index = ArrayValue_insert(&bytecode->values, value);
    assert(value_index > -1);

    if (value_index < 256) {
        Bytecode_insert_instruction_2bytes(
            bytecode,
            OpCode_Stack_Push_Closure,        // OpCode
            (uint8_t)value_index,  // Operand
            line_number,
            DEBUG_TRACE_INSTRUCTION
        );

        return;
    }

    uint8_t byte1 = (value_index >> 0 & 0xff);
    uint8_t byte2 = (value_index >> 8 & 0xff);
    uint8_t byte3 = (value_index >> 16 & 0xff);

    Bytecode_insert_instruction_4bytes(
        bytecode,
        OpCode_Stack_Push_Closure_Long,      // OpCode
        byte1, byte2, byte3,      // Operand
        line_number,
        DEBUG_TRACE_INSTRUCTION
    );
}

// Jumps Forward
int Bytecode_insert_instruction_jump(Bytecode* bytecode, OpCode opcode, int line, bool debug_trace_on) {
    int instruction_start = Bytecode_insert_instruction_1byte(bytecode, opcode, line, false);
    Bytecode_insert_instruction_1byte(bytecode, 0xff, line, false);
    Bytecode_insert_instruction_1byte(bytecode, 0xff, line, false);

    if (debug_trace_on) Bytecode_disassemble_instruction(bytecode, instruction_start);

    return bytecode->instructions.count - 2;
}

// Jumps Backwards
//
void Bytecode_emit_instruction_loop(Bytecode* bytecode, int jump_to_index, int line_number, bool debug_trace_on) {
    // instruction_array_current_position + loop_instruction_size(3 bytes) - increment_index_in_instruction_array
    //
    int instruction_start = bytecode->instructions.count;
    int instruction_length = 3;
    int offset = instruction_start + instruction_length - jump_to_index;
    uint8_t operand_byte1 = ((offset >> 8) & 0xff);
    uint8_t operand_byte2 = (offset & 0xff);

    Bytecode_insert_instruction_1byte(bytecode, OpCode_Loop, line_number, false);
    Bytecode_insert_instruction_1byte(bytecode, operand_byte1, line_number, false);
    Bytecode_insert_instruction_1byte(bytecode, operand_byte2, line_number, false);

    if (debug_trace_on) Bytecode_disassemble_instruction(bytecode, instruction_start);
}

bool Bytecode_patch_instruction_jump(Bytecode* bytecode, int operand_index, bool debug_trace_on) {
    int jump_to_index = bytecode->instructions.count - operand_index - 2;
    if (jump_to_index > UINT16_MAX) return true;

    bytecode->instructions.items[operand_index] = (jump_to_index >> 8) & 0xff;
    bytecode->instructions.items[operand_index + 1] = jump_to_index & 0xff;

    if (debug_trace_on) {
        printf(">> PATCH JUMP:\n");
        Bytecode_disassemble_instruction(bytecode, operand_index - 1);
        printf(">>>>\n");
    }

    return false;
}

static int Bytecode_debug_instruction_byte(const char* opcode_text, int ret_offset_increment)
{
    printf("%s\n", opcode_text);
    return ret_offset_increment;
}

static int Bytecode_debug_instruction_2bytes(Bytecode* bytecode, const char* opcode_text, int ret_offset_increment) {
    uint8_t operand = bytecode->instructions.items[ret_offset_increment - 1];
    Value value = bytecode->values.items[operand];

    if (strcmp(opcode_text, "OPCODE_INTERPOLATION") == 0) {
        printf("%-45s %5s '%d'\n", opcode_text, "**", operand);
    } else {
        printf("%-45s %5d '", opcode_text, operand);
        value_print(value);
        printf("'\n");
    }

    return ret_offset_increment;
}

static int Bytecode_debug_instruction_call(Bytecode* bytecode, const char* opcode_text, int ret_offset_increment) {
    uint8_t operand = bytecode->instructions.items[ret_offset_increment - 1];
    printf("%-45s argc: %d", opcode_text, operand);
    printf("\n");
    return ret_offset_increment;
}

static int Bytecode_debug_instruction_closure(Bytecode* bytecode, const char* opcode_text, int ret_offset_increment) {
    uint8_t operand = bytecode->instructions.items[ret_offset_increment - 1];
    Value value = bytecode->values.items[operand];

    printf("%-45s %5d '", opcode_text, operand);
    value_print(value);
    printf("'\n");

    ObjectFunction* function = value_as_function_object(value);
    for (int i = 0; i < function->variable_dependencies_count; i++) {
        int local_location = bytecode->instructions.items[ret_offset_increment++];
        int index = bytecode->instructions.items[ret_offset_increment++];
        printf("%04d      | %2s %-7s %d\n", ret_offset_increment - 2, "", local_location ? "stack" : "heap", index);
    }

    return ret_offset_increment;
}

static int Bytecode_debug_instruction_local(Bytecode* bytecode, const char* opcode_text, int ret_offset_increment)
{
    uint8_t operand = bytecode->instructions.items[ret_offset_increment - 1];

    printf("%-45s %5d\n", opcode_text, operand);
    return ret_offset_increment;
}

// TODO: rename to bytecode_debug_instruction_jump(bytecode, text, sign, offset);
//
static int Bytecode_debug_instruction_3bytes(Bytecode* bytecode, const char* opcode_text, int ret_offset_increment) {
    uint8_t operand_byte1 = bytecode->instructions.items[ret_offset_increment - 2];
    uint8_t operand_byte2 = bytecode->instructions.items[ret_offset_increment - 1];
    uint16_t operand_value = (uint16_t)((operand_byte1 << 8) | operand_byte2);

    printf("%-45s %5d '", opcode_text, operand_value);
    printf("'\n");

    return ret_offset_increment;
}

static int Bytecode_debug_instruction_jump(Bytecode* bytecode, const char* text, int sign, int ret_offset_increment) {
    uint8_t operand_byte1 = bytecode->instructions.items[ret_offset_increment - 2];
    uint8_t operand_byte2 = bytecode->instructions.items[ret_offset_increment - 1];
    uint16_t operand_value = (uint16_t)((operand_byte1 << 8) | operand_byte2);
    int current_offset = ret_offset_increment - 3;

    printf("%-45s %5d -> %d\n", text, current_offset, ret_offset_increment + operand_value * sign);

    return ret_offset_increment;
}

static int Bytecode_debug_instruction_4bytes(Bytecode* bytecode, const char* opcode_text, int ret_offset_increment)
{
    uint8_t operand_byte1 = bytecode->instructions.items[ret_offset_increment - 3];
    uint8_t operand_byte2 = bytecode->instructions.items[ret_offset_increment - 2];
    uint8_t operand_byte3 = bytecode->instructions.items[ret_offset_increment - 1];
    uint32_t value_index = (uint32_t)((((operand_byte1 << 8) | operand_byte2) << 8) | operand_byte3);
    Value value = bytecode->values.items[value_index];

    printf("%-45s %4d '", opcode_text, value_index);
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
    if (opcode == OpCode_Stack_Push_Literal)
        return Bytecode_debug_instruction_2bytes(bytecode, "OPCODE_STACK_PUSH_LITERAL", (offset + 2));
    if (opcode == OpCode_Stack_Push_Literal_Long)
        return Bytecode_debug_instruction_4bytes(bytecode, "OPCODE_STACK_PUSH_LITERAL_LONG", (offset + 4));
    if (opcode == OpCode_Stack_Push_Literal_Nil)
        return Bytecode_debug_instruction_byte("OPCODE_STACK_PUSH_LITERAL_NIL", (offset + 1));
    if (opcode == OpCode_Stack_Push_Literal_True)
        return Bytecode_debug_instruction_byte("OPCODE_STACK_PUSH_LITERAL_TRUE", (offset + 1));
    if (opcode == OpCode_Stack_Push_Literal_False)
        return Bytecode_debug_instruction_byte("OPCODE_STACK_PUSH_LITERAL_FALSE", (offset + 1));
    if (opcode == OpCode_Stack_Push_Closure)
        return Bytecode_debug_instruction_closure(bytecode, "OPCODE_STACK_PUSH_CLOSURE", (offset + 2));
    if (opcode == OpCode_Stack_Push_Closure_Long)
        return Bytecode_debug_instruction_closure(bytecode, "OPCODE_STACK_PUSH_CLOSURE_LONG", (offset + 4));
    if (opcode == OpCode_Stack_Copy_From_idx_To_Top)
        return Bytecode_debug_instruction_local(bytecode, "OPCODE_STACK_COPY_FROM_IDX_TO_TOP", (offset + 2));
    if (opcode == OpCode_Stack_Copy_Top_To_Idx)
        return Bytecode_debug_instruction_local(bytecode, "OPCODE_STACK_COPY_TOP_TO_IDX", (offset + 2));
    if (opcode == OpCode_Interpolation)
        return Bytecode_debug_instruction_2bytes(bytecode, "OPCODE_INTERPOLATION", (offset + 2));
    if (opcode == OpCode_Stack_Pop)
        return Bytecode_debug_instruction_byte("OPCODE_STACK_POP", (offset + 1));
    if (opcode == OpCode_Define_Global)
        return Bytecode_debug_instruction_2bytes(bytecode, "OPCODE_DEFINE_GLOBAL", (offset + 2));
    if (opcode == OpCode_Read_Global)
        return Bytecode_debug_instruction_2bytes(bytecode, "OPCODE_READ_GLOBAL", (offset + 2));
    if (opcode == OpCode_Class)
        return Bytecode_debug_instruction_2bytes(bytecode, "OPCODE_CLASS", (offset + 2));
    if (opcode == OpCode_Assign_Global)
        return Bytecode_debug_instruction_2bytes(bytecode, "OPCODE_ASSIGN_GLOBAL", (offset + 2));
    if (opcode == OpCode_Copy_From_Stack_To_Heap)
        return Bytecode_debug_instruction_local(bytecode, "OPCODE_COPY_FROM_STACK_TO_HEAP", (offset + 2));
    if (opcode == OpCode_Copy_From_Heap_To_Stack)
        return Bytecode_debug_instruction_local(bytecode, "OPCODE_COPY_FROM_HEAP_TO_STACK", (offset + 2));
    if (opcode == OpCode_Move_Value_To_Heap)
        return Bytecode_debug_instruction_byte("OPCODE_MOVE_VALUE_TO_HEAP", (offset + 1));
    if (opcode == OpCode_Call_Function)
        return Bytecode_debug_instruction_call(bytecode, "OPCODE_CALL_FUNCTION", (offset + 2));
    if (opcode == OpCode_Negation)
        return Bytecode_debug_instruction_byte("OPCODE_NEGATION", (offset + 1));
    if (opcode == OpCode_Not)
        return Bytecode_debug_instruction_byte("OPCODE_NOT", (offset + 1));
    if (opcode == OpCode_Add)
        return Bytecode_debug_instruction_byte("OPCODE_ADD", (offset + 1));
    if (opcode == OpCode_Subtract)
        return Bytecode_debug_instruction_byte("OPCODE_SUBTRACT", (offset + 1));
    if (opcode == OpCode_Multiply)
        return Bytecode_debug_instruction_byte("OPCODE_MULTIPLY", (offset + 1));
    if (opcode == OpCode_Divide)
        return Bytecode_debug_instruction_byte("OPCODE_DIVIDE", (offset + 1));
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
    Bytecode_disassemble_header((char*)name);
    for (int offset = 0; offset < bytecode->instructions.count;) {
        offset = Bytecode_disassemble_instruction(bytecode, offset);
    }
}

void Bytecode_free(Bytecode* bytecode) {
    array_instruction_free(&bytecode->instructions);
    array_line_free(&bytecode->lines);
    ArrayValue_free(&bytecode->values);
}