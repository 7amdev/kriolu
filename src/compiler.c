#include "kriolu.h"

void Compiler_init(Compiler* compiler, FunctionKind function_kind, Compiler** compiler_current) {
    compiler->function = NULL;
    StackLocal_init(&compiler->locals, UINT8_COUNT);
    compiler->function = ObjectFunction_allocate();
    compiler->function_kind = function_kind;
    compiler->depth = 0;

    StackLocal_push(&compiler->locals, (Token) { 0 }, 0);

    // Mark this compiler as global and current
    // 
    *compiler_current = compiler;
}

ObjectFunction* Compiler_end(Compiler* compiler, Compiler** compiler_current, int line_number) {
    ObjectFunction* function = (*compiler_current)->function;

    // emitReturn
    Compiler_CompileInstruction_1Byte(
        &function->bytecode,
        OpCode_Return,
        line_number
    );

#ifdef DEBUG_COMPILER_BYTECODE
    if (!parser->had_error) {
        Bytecode_disassemble(
            &function->bytecode,
            function->name != NULL
            ? function->name->characters
            : "<script>"
        );
    }
#endif


    return function;
    }