#include "kriolu.h"

// TODO: delete this file. The logic has benn moved to the parser module

void Function_init(Function* function, FunctionKind function_kind, Function** function_current, ObjectString* function_name, Object** object_head) {
    function->function_kind = function_kind;
    function->object = NULL;
    function->object = ObjectFunction_allocate(object_head);
    function->object->name = function_name;
    function->depth = 0;
    StackLocal_init(&function->locals, UINT8_COUNT);
    StackLocal_push(&function->locals, (Token) { 0 }, 0, LocalAction_Default);
    ArrayLocalMetadata_init(&function->variable_dependencies, 0);

    // TODO: LinkedList_push(function_current, function);
    //
    function->next = *function_current;  // Saves the current Function stored in parser->function
    *function_current = function;            // Mark this function as global and current
}

ObjectFunction* Function_end(Function* function, Function** function_current, bool parser_has_error, int line_number) {
    ObjectFunction* object_fn = (*function_current)->object;

    // TODO: parser_compile_return(...);
    // REVIEW
    Compiler_CompileInstruction_1Byte(
        &object_fn->bytecode,
        OpCode_Stack_Push_Literal_Nil,
        line_number
    );

    Compiler_CompileInstruction_1Byte(
        &object_fn->bytecode,
        OpCode_Return,
        line_number
    );

    // NOTE: This code pop a item from the list making a memory leak, because we lost 
    //       the reference of the object to free it.
    //
    *function_current = (*function_current)->next;

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

    return object_fn;
}

// NOTE: If a closure (child function) access a variable from a parent, then it will
//       traverse all the parent hierarchy to find the local. Once its found,
//       the variable will be copyed in all the function the traversal had to
//       to go trough to find the variabe. 
//
int Function_resolve_variable_dependencies(Function* function, Token* name, Local** ret_local) {
    // TODO: make it 'static', then clear on return
    StackFunction stack_function = { 0 };
    int local_found_idx = -1;

    StackFunction_push(&stack_function, function);

    Function* current_function = function->next;
    for (;;) {
        if (current_function == NULL) break;

        local_found_idx = StackLocal_get_local_index_by_token(&current_function->locals, name, NULL);
        if (local_found_idx != -1) break; // if found token, break ot of thhe loop

        StackFunction_push(&stack_function, current_function);

        current_function = current_function->next;
    }

    // if (local_found_idx == -1) {
    //     StackTemp_clear();
    //     return -1;
    // }
    //
    if (local_found_idx == -1) return -1;

    current_function->locals.items[local_found_idx].action = LocalAction_Move_Heap;
    *ret_local = &current_function->locals.items[local_found_idx];

    Function* top_function = StackFunction_pop(&stack_function);
    int variable_dependency_idx = ArrayLocalMetadata_add(
        &top_function->variable_dependencies,
        (uint8_t)local_found_idx,
        LocalLocation_In_Parent_Stack,
        &top_function->object->variable_dependencies_count
    );

    if (StackFunction_is_empty(&stack_function)) return variable_dependency_idx;

    // TODO: rename to 'var_dependency_prior'
    int var_dependency_latest = variable_dependency_idx;
    for (;;) {
        if (StackFunction_is_empty(&stack_function)) break;
        if (variable_dependency_idx == -1) break;

        top_function = StackFunction_peek(&stack_function, 0);
        var_dependency_latest = ArrayLocalMetadata_add(
            &top_function->variable_dependencies,
            (uint8_t)var_dependency_latest,
            LocalLocation_In_Parent_Heap_Values,
            &top_function->object->variable_dependencies_count
        );

        StackFunction_pop(&stack_function);
    }

    return var_dependency_latest;
}

// FUNCTION: Function_Resolve_Variable_Dependencies
//           Recursive Implementation
//
// int Function_resolve_variable_dependencies(Function* function, Token* name, Local** out) {
//     if (function->previous == NULL) return -1;
//
//     // Search for the variable one level up, or on the enclosing function locals 
//     // In Runtime, the enclosing function's locals are in the stack, thats why
//     // i called it 'stack_index'.
//     // 
//     int stack_index = StackLocal_get_local_index_by_token(&function->previous->locals, name, out);
//     if (stack_index != -1) {
//         function->previous->locals.items[stack_index].action = LocalAction_Move_Heap;
//         return ArrayLocalMetadata_add(
//             &function->variable_dependencies,
//             (uint8_t)stack_index,
//             LocalLocation_In_Parent_Stack,
//             &function->function->variable_dependencies_count
//         );
//     }
//
//     int heap_value_index = Function_resolve_variable_dependencies(function->previous, name, out);
//     if (heap_value_index != -1) {
//         return ArrayLocalMetadata_add(
//             &function->variable_dependencies,
//             (uint8_t)heap_value_index,
//             LocalLocation_In_Parent_Heap_Values,
//             &function->function->variable_dependencies_count
//         );
//     }
//
//     return -1;
//
// }