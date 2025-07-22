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
    StackLocal_push(&compiler->locals, (Token) { 0 }, 0, LocalAction_Default);
    ArrayLocalMetadata_init(&compiler->variable_dependencies, 0);

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

// NOTE: If a closure (child function) access a variable from a parent, then it will
//       traverse all the parent hierarchy to find the local. Once its found,
//       the variable will be copyed in all the function the traversal had to
//       to go trough to find the variabe. 
//
int Compiler_resolve_variable_dependencies(Compiler* compiler, Token* name, Local** out) {
    if (compiler->previous == NULL) return -1;

    // Search for the variable one level up, or on the enclosing function locals 
    // In Runtime, the enclosing function's locals are in the stack, thats why
    // i called it 'stack_index'.
    // 
    int stack_index = StackLocal_get_local_index_by_token(&compiler->previous->locals, name, out);
    if (stack_index != -1) {
        compiler->previous->locals.items[stack_index].action = LocalAction_Move_Heap;
        return ArrayLocalMetadata_add(
            &compiler->variable_dependencies,
            (uint8_t)stack_index,
            LocalLocation_In_Parent_Stack,
            &compiler->function->variable_dependencies_count
        );
    }

    int heap_value_index = Compiler_resolve_variable_dependencies(compiler->previous, name, out);
    if (heap_value_index != -1) {
        return ArrayLocalMetadata_add(
            &compiler->variable_dependencies,
            (uint8_t)heap_value_index,
            LocalLocation_In_Parent_Heap_Values,
            &compiler->function->variable_dependencies_count
        );
    }

    return -1;
}