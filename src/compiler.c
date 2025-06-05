#include "kriolu.h"

void Compiler_init(Compiler* compiler, FunctionKind function_kind, Compiler** compiler_current) {
    compiler->function = NULL;
    scope_init(&compiler->scope);
    compiler->function = ObjectFunction_allocate();
    compiler->function_kind = function_kind;

    StackLocal_push(&compiler->scope.locals, (Token) { 0 }, 0);
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