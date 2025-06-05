#include "../../src/kriolu.h"

int main(void)
{
    Bytecode bytecode;
    Bytecode_init(&bytecode);

    for (int i = 0; i < 260; i++) {
        Value value = value_make_number(i);
        Bytecode_insert_instruction_constant(&bytecode, value, 123);
    }

    Bytecode_insert_instruction_1byte(&bytecode, OpCode_Return, 123);

    Bytecode_disassemble(&bytecode, "Test");

    Bytecode_free(&bytecode);
    return 0;
}