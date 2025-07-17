#include "kriolu.h"

// TODO: rename file to parser_function.c
// TODO: rename type prefix to ParserFunction_init 

void Compiler_init(Compiler* compiler, FunctionKind function_kind, Compiler** compiler_current, ObjectString* function_name, Object** object_head) {
    compiler->previous = *compiler_current; // Saves the current Compiler stored in parser->compiler
    compiler->function = NULL;
    compiler->function = ObjectFunction_allocate(object_head);
    compiler->function_kind = function_kind;
    compiler->depth = 0;
    compiler->function->name = function_name;

    StackLocal_init(&compiler->locals, UINT8_COUNT);
    StackLocal_push(&compiler->locals, (Token) { 0 }, 0, false);
    ArrayUpvalue_init(&compiler->upvalues, 0);

    // Mark this compiler as global and current
    // 
    *compiler_current = compiler;
}

ObjectFunction* Compiler_end(Compiler* compiler, Compiler** compiler_current, bool parser_has_error, int line_number) {
    ObjectFunction* function = (*compiler_current)->function;

    // TODO: parser_compile_return(...);
    // REVIEW
    Compiler_CompileInstruction_1Byte(
        &function->bytecode,
        OpCode_Nil,
        line_number
    );

    Compiler_CompileInstruction_1Byte(
        &function->bytecode,
        OpCode_Return,
        line_number
    );

    // Restore parser->compiler to the previous state
    //
    *compiler_current = (*compiler_current)->previous;

#ifdef DEBUG_COMPILER_BYTECODE
    if (!parser_has_error) {
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

int Compiler_resolve_upvalues(Compiler* compiler, Token* name, Local** out) {
    if (compiler->previous == NULL) return -1;

    // Search for the variable one level up, or on the enclosing function locals 
    //
    int local_index = StackLocal_get_local_index_by_token(&compiler->previous->locals, name, out);
    if (local_index != -1) {
        // NOTE: Mark Local to be moved to the Heap
        // TODO: rename 'is_capture' to 'local_source'
        //
        compiler->previous->locals.items[local_index].is_captured = true;

        return ArrayUpvalue_add(
            &compiler->upvalues,
            (uint8_t)local_index,
            true,
            &compiler->function->upvalue_count
        );
    }

    int upvalue = Compiler_resolve_upvalues(compiler->previous, name, out);
    if (upvalue != -1) {
        return ArrayUpvalue_add(
            &compiler->upvalues,
            (uint8_t)upvalue,
            false,
            &compiler->function->upvalue_count
        );
    }

    return -1;
}