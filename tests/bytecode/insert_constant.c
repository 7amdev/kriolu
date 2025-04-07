#include "../../src/kriolu.h"

int main(void)
{
    Bytecode bytecode;
    bytecode_init(&bytecode);

    for (int i = 0; i < 260; i++) {
        Value value = value_make_number(i);
        bytecode_insert_instruction_constant(&bytecode, value, 123);
    }

    bytecode_insert_instruction_1byte(&bytecode, OpCode_Return, 123);

    bytecode_disassemble(&bytecode, "Test");

    bytecode_free(&bytecode);
    return 0;
}