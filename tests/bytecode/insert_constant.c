#include "../../src/kriolu.h"

int main(void)
{
    Bytecode bytecode;
    bytecode_init(&bytecode);

    for (int i = 0; i < 260; i++)
        bytecode_write_constant(&bytecode, i, 123);

    bytecode_write_opcode(&bytecode, OpCode_Return, 123);

    bytecode_disassemble(&bytecode, "Test");

    bytecode_free(&bytecode);
    return 0;
}