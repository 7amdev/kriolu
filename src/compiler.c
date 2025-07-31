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
int Compiler_resolve_variable_dependencies(Compiler* compiler, Token* name, Local** ret_local) {
    // TODO: make it 'static', then clear on return
    StackCompiler stack_compiler = { 0 };
    int local_found_idx = -1;

    StackCompiler_push(&stack_compiler, compiler);

    Compiler* current_compiler = compiler->previous;
    for (;;) {
        if (current_compiler == NULL) break;

        local_found_idx = StackLocal_get_local_index_by_token(&current_compiler->locals, name, NULL);
        if (local_found_idx != -1) break; // if found token, break ot of thhe loop

        StackCompiler_push(&stack_compiler, current_compiler);

        current_compiler = current_compiler->previous;
    }

    // if (local_found_idx == -1) {
    //     StackTemp_clear();
    //     return -1;
    // }
    //
    if (local_found_idx == -1) return -1;

    current_compiler->locals.items[local_found_idx].action = LocalAction_Move_Heap;
    *ret_local = &current_compiler->locals.items[local_found_idx];

    Compiler* top_compiler = StackCompiler_pop(&stack_compiler);
    int variable_dependency_idx = ArrayLocalMetadata_add(
        &top_compiler->variable_dependencies,
        (uint8_t)local_found_idx,
        LocalLocation_In_Parent_Stack,
        &top_compiler->function->variable_dependencies_count
    );

    if (StackCompiler_is_empty(&stack_compiler)) return variable_dependency_idx;

    // TODO: rename to 'var_dependency_prior'
    int var_dependency_latest = variable_dependency_idx;
    for (;;) {
        if (StackCompiler_is_empty(&stack_compiler)) break;
        if (variable_dependency_idx == -1) break;

        top_compiler = StackCompiler_peek(&stack_compiler, 0);
        var_dependency_latest = ArrayLocalMetadata_add(
            &top_compiler->variable_dependencies,
            (uint8_t)var_dependency_latest,
            LocalLocation_In_Parent_Heap_Values,
            &top_compiler->function->variable_dependencies_count
        );

        StackCompiler_pop(&stack_compiler);
    }

    return var_dependency_latest;
}

// FUNCTION: Compiler_Resolve_Variable_Dependencies
//           Recursive Implementation
//
// int Compiler_resolve_variable_dependencies(Compiler* compiler, Token* name, Local** out) {
//     if (compiler->previous == NULL) return -1;
//
//     // Search for the variable one level up, or on the enclosing function locals 
//     // In Runtime, the enclosing function's locals are in the stack, thats why
//     // i called it 'stack_index'.
//     // 
//     int stack_index = StackLocal_get_local_index_by_token(&compiler->previous->locals, name, out);
//     if (stack_index != -1) {
//         compiler->previous->locals.items[stack_index].action = LocalAction_Move_Heap;
//         return ArrayLocalMetadata_add(
//             &compiler->variable_dependencies,
//             (uint8_t)stack_index,
//             LocalLocation_In_Parent_Stack,
//             &compiler->function->variable_dependencies_count
//         );
//     }
//
//     int heap_value_index = Compiler_resolve_variable_dependencies(compiler->previous, name, out);
//     if (heap_value_index != -1) {
//         return ArrayLocalMetadata_add(
//             &compiler->variable_dependencies,
//             (uint8_t)heap_value_index,
//             LocalLocation_In_Parent_Heap_Values,
//             &compiler->function->variable_dependencies_count
//         );
//     }
//
//     return -1;
//
// }