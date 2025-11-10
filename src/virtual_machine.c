#include "kriolu.h"

//
// Globals
//
// VirtualMachine g_vm;

//
// Forward Declaration 
// 

void VirtualMachine_runtime_error(VirtualMachine* vm, const char* format, ...);
static void VirtualMachine_define_function_native(VirtualMachine* vm, const char* function_name, FunctionNative* function, int arity);
static bool VirtualMachine_call_closure(VirtualMachine* vm, ObjectClosure* closure, int argument_count);
static bool VirtualMachine_call_value(VirtualMachine* vm, Value function, int argument_count);
static bool VirtualMachine_call_method(VirtualMachine* vm, ObjectString* name, int argument_count);
static bool VirtualMachine_call_from_class(VirtualMachine* vm, ObjectClass* klass, ObjectString* method_name, int argument_count);
static ObjectValue* VirtualMachine_create_heap_value(VirtualMachine* vm, Value* value_address);
static void VirtualMachine_move_value_from_stack_to_heap(VirtualMachine* vm, Value* value_address);
static Value FunctionNative_clock(VirtualMachine* vm, int argument_count, Value* arguments);

// TODO: delete code bellow
//
// static inline bool Object_check_value_kind(Value value, ObjectKind object_kind);

//
// Function Declarations
//
static ObjectString* VirtualMachine_intern_string(VirtualMachine* vm, String string) {
    uint32_t hash = string_hash(string);
    ObjectString* result = ObjectString_Allocate(
        .task = (
            AllocateTask_Check_If_Interned  |
            AllocateTask_Copy_String        |
            AllocateTask_Initialize         |
            AllocateTask_Intern
        ),
        .string = string,
        .hash   = hash,
        .first  = &vm->objects,
        .table  = &vm->string_database
    );

    return result;
}

void VirtualMachine_init(VirtualMachine* vm) {
    String konstrutor      = string_make("konstrutor", 10);

    vm->objects            = NULL;
    vm->heap_values        = NULL;
    vm->object_init_text   = "konstrutor";
    stack_value_reset(&vm->stack_value);
    StackFunctionCall_reset(&vm->function_calls);
    hash_table_init(&vm->global_database);
    hash_table_init(&vm->string_database);

    Memory_register(NULL, vm, NULL);

    vm->object_init_string = VirtualMachine_intern_string(vm, konstrutor);
    VirtualMachine_define_function_native(vm, "rilogio", &FunctionNative_clock, 0);
}


InterpreterResult VirtualMachine_interpret(VirtualMachine* vm, ObjectFunction* script) {
    if (script == NULL) return Interpreter_Function_error;

    Value value = value_make_object(script);
    stack_value_push(&vm->stack_value, value);
    ObjectClosure* closure = ObjectClosure_allocate(script, &vm->objects);
    stack_value_pop(&vm->stack_value);
    Value v_closure = value_make_object(closure);
    stack_value_push(&vm->stack_value, v_closure);

    FunctionCall* current_function_call = StackFunctionCall_push(
        &vm->function_calls,
        closure,
        vm->stack_value.top,
        0
    );

#define READ_BYTE_THEN_INCREMENT() (*current_function_call->ip++) // uint8_t instruction = (vm->ip += 1, vm->ip[-1])
#define READ_2BYTE() (current_function_call->ip += 2, (uint16_t)((current_function_call->ip[-2] << 8) | current_function_call->ip[-1])) 
#define READ_3BYTE_THEN_INCREMENT() (((((*current_function_call->ip++) << 8) | (*current_function_call->ip++)) << 8) | (*current_function_call->ip++))
#define READ_CONSTANT() (current_function_call->closure->function->bytecode.values.items[READ_BYTE_THEN_INCREMENT()])
#define READ_CONSTANT_3BYTE() (current_function_call->closure->function->bytecode.values.items[READ_3BYTE_THEN_INCREMENT()])
#define READ_STRING() value_as_string(READ_CONSTANT())
    // TODO: #define READ_STRING_3BYTE() (...)

#ifdef DEBUG_TRACE_EXECUTION
    ObjectString* function_name = current_function_call->closure->function->name;
    char* title = function_name == NULL ? "Script" : function_name->characters;
    Bytecode_disassemble_header(title);
#endif 

    for (;;)
    {

#ifdef DEBUG_TRACE_EXECUTION
        stack_value_trace(&vm->stack_value);
        Bytecode_disassemble_instruction(
            &current_function_call->closure->function->bytecode,
            (int)(current_function_call->ip - current_function_call->closure->function->bytecode.instructions.items)
        );
#endif

        uint8_t instruction = READ_BYTE_THEN_INCREMENT();
        switch (instruction)
        {
        default:
        {
            assert(false && "Unsupported OpCode. Handle the OpCode by adding a if statement.");
        }
        case OpCode_Stack_Push_Literal:
        {
            Value constant = READ_CONSTANT();
            stack_value_push(&vm->stack_value, constant);
            break;
        }
        case OpCode_Stack_Push_Literal_Long:
        {
            Value constant = READ_CONSTANT_3BYTE();
            stack_value_push(&vm->stack_value, constant);
            break;
        }
        case OpCode_Stack_Push_Closure:
        {
            ObjectFunction* function = value_as_function_object(READ_CONSTANT());
            ObjectClosure* closure = ObjectClosure_allocate(function, &vm->objects);
            stack_value_push(&vm->stack_value, value_make_object(closure));
            for (int i = 0; i < closure->function->variable_dependencies_count; i++) {
                uint8_t local_location       = READ_BYTE_THEN_INCREMENT();
                uint8_t local_location_index = READ_BYTE_THEN_INCREMENT();
                if (local_location == LocalLocation_In_Parent_Stack) {
                    closure->heap_values.items[i] = VirtualMachine_create_heap_value(
                        vm,
                        current_function_call->frame_start + local_location_index
                    );
                } 
                else if (local_location == LocalLocation_In_Parent_Heap_Values) { 
                    closure->heap_values.items[i] = current_function_call->closure->heap_values.items[local_location_index];
                }
                else {
//                  TODO: review error message
                    VirtualMachine_runtime_error(vm, "Could not 'Close' the variable: invalid location.");
                }
            }
            break;
        }
        case OpCode_Stack_Push_Closure_Long:
        {
            ObjectFunction* function = value_as_function_object(READ_CONSTANT_3BYTE());
            ObjectClosure* closure = ObjectClosure_allocate(function, &vm->objects);
            stack_value_push(&vm->stack_value, value_make_object(closure));
            for (int i = 0; i < closure->function->variable_dependencies_count; i++) {
                uint8_t local_location = READ_BYTE_THEN_INCREMENT(); // TODO: rename to 'local_location'
                uint8_t local_location_index = READ_BYTE_THEN_INCREMENT();
                if (local_location == LocalLocation_In_Parent_Stack) { // TODO: change line to 'if(local_location == LocalLocation_In_Parent_Stack) {...}'
                    closure->heap_values.items[i] = VirtualMachine_create_heap_value(vm, current_function_call->frame_start + local_location_index);
                } 
                else if (local_location == LocalLocation_In_Parent_Heap_Values) {
                    closure->heap_values.items[i] = current_function_call->closure->heap_values.items[local_location_index];
                }
                else {
                    VirtualMachine_runtime_error(vm, "Could not Close the variable: invalid location.");
                }
            }
            break;
        }
        case OpCode_Stack_Push_Literal_True:
        {
            stack_value_push(&vm->stack_value, value_make_boolean(true));
            break;
        }
        case OpCode_Stack_Push_Literal_False:
        {
            stack_value_push(&vm->stack_value, value_make_boolean(false));
            break;
        }
        case OpCode_Stack_Push_Literal_Nil:
        {
            stack_value_push(&vm->stack_value, value_make_nil());
            break;
        }
        case OpCode_Stack_Copy_From_idx_To_Top:
        {
            uint8_t local_slot_index = READ_BYTE_THEN_INCREMENT();

            Value local = current_function_call->frame_start[local_slot_index];
            stack_value_push(&vm->stack_value, local);

            break;
        }
        case OpCode_Stack_Copy_Top_To_Idx:
        {
            uint8_t local_slot_index = READ_BYTE_THEN_INCREMENT();

            current_function_call->frame_start[local_slot_index] = stack_value_peek(&vm->stack_value, 0);
            break;
        }
        case OpCode_Stack_Move_Value_To_Heap: {
            VirtualMachine_move_value_from_stack_to_heap(vm, vm->stack_value.top - 1);
            stack_value_pop(&vm->stack_value);
            break;
        }
        case OpCode_Stack_Move_Top_To_Heap: {
            uint8_t index = READ_BYTE_THEN_INCREMENT();
            *current_function_call->closure->heap_values.items[index]->value_address = stack_value_peek(&vm->stack_value, 0);
            break;
        }
        case OpCode_Stack_Copy_From_Heap_To_Top: {
            uint8_t index = READ_BYTE_THEN_INCREMENT();
            stack_value_push(
                &vm->stack_value,
                *current_function_call->closure->heap_values.items[index]->value_address
            );
            break;
        }
        case OpCode_Stack_Pop:
        {
            stack_value_pop(&vm->stack_value);
            break;
        }
        case OpCode_Define_Global:
        {
            ObjectString* variable_name = READ_STRING();
            Value value = stack_value_peek(&vm->stack_value, 0);
            hash_table_set_value(&vm->global_database, variable_name, value);
            stack_value_pop(&vm->stack_value);
            break;
        }
        case OpCode_Read_Global:
        {
            ObjectString* variable_name = READ_STRING();
            Value value;

            if (hash_table_get_value(&vm->global_database, variable_name, &value) == false) {
                VirtualMachine_runtime_error(vm, "Undefined variable '%s'.", variable_name->characters);
                return Interpreter_Runtime_Error;
            }

            stack_value_push(&vm->stack_value, value);
            break;
        }
        case OpCode_Assign_Global:
        {
            ObjectString* variable_name = READ_STRING();
            Value value = stack_value_peek(&vm->stack_value, 0);
            bool is_new = hash_table_set_value(&vm->global_database, variable_name, value);
            if (is_new) {
                hash_table_delete(&vm->global_database, variable_name);
                VirtualMachine_runtime_error(vm, "Undefined variable '%s'.", variable_name->characters);
                return Interpreter_Runtime_Error;
            }
            break;
        }
        case OpCode_Call_Function:
        {
            int argument_count = READ_BYTE_THEN_INCREMENT();
            Value function = stack_value_peek(&vm->stack_value, argument_count);
            if (value_as_object(function)->kind == ObjectKind_Class) {
                VirtualMachine_runtime_error(vm, "Expect a 'Funson' to call.\n-- Did you meant '%s{}'", value_as_class(function)->name->characters);
                return Interpreter_Runtime_Error;
            }

            if (!VirtualMachine_call_value(vm, function, argument_count)) {
                return Interpreter_Runtime_Error;
            }

            current_function_call = StackFunctionCall_peek(&vm->function_calls, 0);
            break;
        }
        case OpCode_Call_Class: {
            int argument_count = READ_BYTE_THEN_INCREMENT();
            Value function = stack_value_peek(&vm->stack_value, argument_count);
            if (value_as_object(function)->kind != ObjectKind_Class) {
                VirtualMachine_runtime_error(vm, "Expect a Class to call the 'konstrutor'.");
                return Interpreter_Runtime_Error;
            }

            if (!VirtualMachine_call_value(vm, function, argument_count)) {
                return Interpreter_Runtime_Error;
            }

            current_function_call = StackFunctionCall_peek(&vm->function_calls, 0);
            break;
            break;
        }
        case OpCode_Call_Method: 
        {
            ObjectString* method_name = READ_STRING();
            int argument_count = READ_BYTE_THEN_INCREMENT();
            if (!VirtualMachine_call_method(vm, method_name, argument_count)) {
                return Interpreter_Runtime_Error;
            }
            
            current_function_call = StackFunctionCall_peek(&vm->function_calls, 0);
            break;
        }
        case OpCode_Call_Super_Method:
        {
            ObjectString* method_name = READ_STRING();
            int argument_count = READ_BYTE_THEN_INCREMENT();
            ObjectClass* superclass = value_as_class(stack_value_pop(&vm->stack_value));
            if (!VirtualMachine_call_from_class(vm, superclass, method_name, argument_count)) {
                return Interpreter_Runtime_Error;
            }

            current_function_call = StackFunctionCall_peek(&vm->function_calls, 0);
            break;
        }
//      TODO: rename to 'OpCode_Stack_Push_Class'
        case OpCode_Class:
        {
            ObjectClass* klass = ObjectClass_alocate(READ_STRING(), &vm->objects);
            Value klass_value = value_make_object(klass);
            stack_value_push(&vm->stack_value, klass_value);
            break;
        }
//      TODO: rename to 'OpCode_Attach_Method_To_Class'
        case OpCode_Method: 
        {
            ObjectString* method_name = READ_STRING();
            Value closure_method = stack_value_peek(&vm->stack_value, 0);
            ObjectClass* klass = value_as_class(stack_value_peek(&vm->stack_value, 1));
            hash_table_set_value(&klass->methods, method_name, closure_method);
            stack_value_pop(&vm->stack_value);
            break;
        }
        case OpCode_Inheritance:
        {
            ObjectClass* subclass = value_as_class(stack_value_peek(&vm->stack_value, 0));
            Value superclass = stack_value_peek(&vm->stack_value, 1);
            if (!value_is_class(superclass)) 
                VirtualMachine_runtime_error(vm, "Superclass must be a class.");
            
            hash_table_copy(&value_as_class(superclass)->methods, &subclass->methods);
            stack_value_pop(&vm->stack_value);
            break;
        }
        case OpCode_Get_Super: 
        {
            ObjectString* method_name = READ_STRING();
            ObjectClass* superclass = value_as_class(stack_value_pop(&vm->stack_value));

            Value closure_method;
            if (hash_table_get_value(&superclass->methods, method_name, &closure_method)) {
                ObjectMethod* obj_method = ObjectMethod_allocate(
                    stack_value_peek(&vm->stack_value, 0), 
                    value_as_closure(closure_method),
                    &vm->objects
                );

                stack_value_pop(&vm->stack_value); // NOTE: Pop the Instance
                stack_value_push(&vm->stack_value, value_make_object_method(obj_method));
                break;
            }

            VirtualMachine_runtime_error(vm, "Undefined property '%s'.", method_name->characters);
            return Interpreter_Runtime_Error;
        }
        case OpCode_Object_Get_Property:
        {
            if (!value_is_instance(stack_value_peek(&vm->stack_value, 0))) {
                VirtualMachine_runtime_error(vm, "Only instances have properties.");
                return Interpreter_Runtime_Error;
            }

            ObjectInstance* obj_instance = value_as_instance(stack_value_peek(&vm->stack_value, 0));
            ObjectString* property_name  = READ_STRING();

//          First, It looks for 'property-name' in the Instance's fields hash-table, and 
//          if it didn't find any item, It searchs in the Class's methods hash-table.
            Value value;
            if (hash_table_get_value(&obj_instance->fields, property_name, &value)) {
                stack_value_pop(&vm->stack_value); // NOTE: Pop the Instance
                stack_value_push(&vm->stack_value, value);
                break;
            }
            
            Value closure_method;
            if (hash_table_get_value(&obj_instance->klass->methods, property_name, &closure_method)) {
                ObjectMethod* obj_method = ObjectMethod_allocate(
                    stack_value_peek(&vm->stack_value, 0), 
                    value_as_closure(closure_method),
                    &vm->objects
                );

                stack_value_pop(&vm->stack_value); // NOTE: Pop the Instance
                stack_value_push(&vm->stack_value, value_make_object_method(obj_method));
                break;
            }

            VirtualMachine_runtime_error(vm, "Undefined property '%s'.", property_name->characters);
            return Interpreter_Runtime_Error;
        }
        case OpCode_Object_Set_Property:
        {
            if (!value_is_instance(stack_value_peek(&vm->stack_value, 1))) {
                VirtualMachine_runtime_error(vm, "Only instances have properties.");
                return Interpreter_Runtime_Error;
            }

            ObjectInstance* instance      = value_as_instance(stack_value_peek(&vm->stack_value, 1));
            ObjectString*   property_name = READ_STRING();

            hash_table_set_value(
                &instance->fields, 
                property_name, 
                stack_value_peek(&vm->stack_value, 0)
            );

            Value value = stack_value_pop(&vm->stack_value);
            stack_value_pop(&vm->stack_value);  // NOTE: Pop the Instance
            stack_value_push(&vm->stack_value, value);
    
            break;
        }
        case OpCode_Negation:
        {
            if (!value_is_number(stack_value_peek(&vm->stack_value, 0)))
            {
                VirtualMachine_runtime_error(vm, "Operand must be a number.");
                return Interpreter_Runtime_Error;
            }

            Value value = stack_value_pop(&vm->stack_value);
            double number = value_as_number(value);
            Value value_negated = value_make_number(-(number));

            stack_value_push(&vm->stack_value, value_negated);
            break;
        }
        case OpCode_Not:
        {
            bool result = value_negate_logically(stack_value_pop(&vm->stack_value));
            Value value = value_make_boolean(result);

            stack_value_push(&vm->stack_value, value);
            break;
        }
        case OpCode_Interpolation:
        {
            // How pop value should a make to retrieve all the values necessary
            // to join.
            //
            uint8_t total_pop_from_stack = READ_BYTE_THEN_INCREMENT();

            // TODO: missing implementation
            //       1) pop values from the stack
            //       2) convert any integer to string
            //       3) compute the total size and allocate the memory
            //       4) copy
            //       5) push to stack
            //
            assert(false && "TODO: missing implementation");
            break;
        }
        case OpCode_Add:
        {
            if (
                value_is_number(stack_value_peek(&vm->stack_value, 0)) &&
                value_is_number(stack_value_peek(&vm->stack_value, 1))
            ) {
                Value b = stack_value_pop(&vm->stack_value);
                Value a = stack_value_pop(&vm->stack_value);
                double sum = value_as_number(a) + value_as_number(b);
                Value value_sum = value_make_number(sum);

                stack_value_push(&vm->stack_value, value_sum);
            } 
            else if (
                value_is_string(stack_value_peek(&vm->stack_value, 0)) &&
                value_is_string(stack_value_peek(&vm->stack_value, 1))
            ) {
                Value b             = stack_value_peek(&vm->stack_value, 0);
                Value a             = stack_value_peek(&vm->stack_value, 1);
                ObjectString* os_a  = value_as_string(a);
                ObjectString* os_b  = value_as_string(b);
                String s_a          = string_make(os_a->characters, os_a->length);
                String s_b          = string_make(os_b->characters, os_b->length);

                String final        = string_concatenate(s_a, s_b);
                uint32_t final_hash = string_hash(final);

                ObjectString* object_st = hash_table_get_key(&vm->string_database, final, final_hash);
                if (object_st == NULL) {
                    object_st = ObjectString_Allocate(
                        .task   = AllocateTask_Initialize | AllocateTask_Intern,
                        .string = final,
                        .hash   = final_hash,
                        .first  = &vm->objects,
                        .table  = &vm->string_database
                    );
                } 
                else {
                    string_free(&final);
                }

                stack_value_pop(&vm->stack_value);
                stack_value_pop(&vm->stack_value);
                stack_value_push(&vm->stack_value, value_make_object(object_st));
            } else {
                VirtualMachine_runtime_error(vm, "Operands must be 2(two) numbers or 2(two) strings.");
                return Interpreter_Runtime_Error;
            }

            break;
        }
        case OpCode_Subtract:
        {
            if (!value_is_number(stack_value_peek(&vm->stack_value, 0)) ||
                !value_is_number(stack_value_peek(&vm->stack_value, 1)))
            {
                VirtualMachine_runtime_error(vm, "Operands must be numbers.");
                return Interpreter_Runtime_Error;
            }

            Value b = stack_value_pop(&vm->stack_value);
            Value a = stack_value_pop(&vm->stack_value);
            double difference = value_as_number(a) - value_as_number(b);
            Value value_difference = value_make_number(difference);

            stack_value_push(&vm->stack_value, value_difference);
            break;
        }
        case OpCode_Multiply:
        {
            if (!value_is_number(stack_value_peek(&vm->stack_value, 0)) ||
                !value_is_number(stack_value_peek(&vm->stack_value, 1)))
            {
                VirtualMachine_runtime_error(vm, "Operands must be numbers.");
                return Interpreter_Runtime_Error;
            }

            Value b = stack_value_pop(&vm->stack_value);
            Value a = stack_value_pop(&vm->stack_value);
            double product = value_as_number(a) * value_as_number(b);
            Value value_product = value_make_number(product);

            stack_value_push(&vm->stack_value, value_product);
            break;
        }
        case OpCode_Divide:
        {
            if (!value_is_number(stack_value_peek(&vm->stack_value, 0)) ||
                !value_is_number(stack_value_peek(&vm->stack_value, 1)))
            {
                VirtualMachine_runtime_error(vm, "Operands must be numbers.");
                return Interpreter_Runtime_Error;
            }

            Value b = stack_value_pop(&vm->stack_value);
            Value a = stack_value_pop(&vm->stack_value);
            double quotient = value_as_number(a) / value_as_number(b);
            Value value_quotient = value_make_number(quotient);

            stack_value_push(&vm->stack_value, value_quotient);
            break;
        }
        case OpCode_Exponentiation:
        {
            if (!value_is_number(stack_value_peek(&vm->stack_value, 0)) ||
                !value_is_number(stack_value_peek(&vm->stack_value, 1)))
            {
                VirtualMachine_runtime_error(vm, "Operands must be numbers.");
                return Interpreter_Runtime_Error;
            }

            Value b = stack_value_pop(&vm->stack_value);
            Value a = stack_value_pop(&vm->stack_value);
            double power = pow(value_as_number(a), value_as_number(b));
            Value value_power = value_make_number(power);

            stack_value_push(&vm->stack_value, value_power);
            break;
        }
        case OpCode_Equal_To:
        {
            Value b = stack_value_pop(&vm->stack_value);
            Value a = stack_value_pop(&vm->stack_value);
            bool is_equal = value_is_equal(a, b);
            stack_value_push(&vm->stack_value, value_make_boolean(is_equal));
            break;
        }
        case OpCode_Greater_Than:
        {
            if (!value_is_number(stack_value_peek(&vm->stack_value, 0)) ||
                !value_is_number(stack_value_peek(&vm->stack_value, 1)))
            {
                VirtualMachine_runtime_error(vm, "Operands must be numbers.");
                return Interpreter_Runtime_Error;
            }

            Value b = stack_value_pop(&vm->stack_value);
            Value a = stack_value_pop(&vm->stack_value);
            bool result = (value_as_number(a) > value_as_number(b));

            stack_value_push(&vm->stack_value, value_make_boolean(result));
            break;
        }
        case OpCode_Less_Than:
        {
            if (!value_is_number(stack_value_peek(&vm->stack_value, 0)) ||
                !value_is_number(stack_value_peek(&vm->stack_value, 1)))
            {
                VirtualMachine_runtime_error(vm, "Operands must be numbers.");
                return Interpreter_Runtime_Error;
            }

            Value b = stack_value_pop(&vm->stack_value);
            Value a = stack_value_pop(&vm->stack_value);
            bool result = (value_as_number(a) < value_as_number(b));

            stack_value_push(&vm->stack_value, value_make_boolean(result));
            break;
        }
        case OpCode_Print:
        {
            Value value = stack_value_pop(&vm->stack_value);
            value_print(value);
            printf("\n");
            break;
        }
        case OpCode_Jump_If_False:
        {
            uint16_t offset = READ_2BYTE(); // TODO: change name to INCREMENT_BY_2BYTES_THEN_READ()
            if (value_is_falsey(stack_value_peek(&vm->stack_value, 0)))
                current_function_call->ip += offset;

            break;
        }
        case OpCode_Jump:
        {
            uint16_t offset = READ_2BYTE();
            current_function_call->ip += offset;
            break;
        }
        case OpCode_Loop:
        {
            uint16_t offset = READ_2BYTE();
            current_function_call->ip -= offset;
            break;
        }
        case OpCode_Return:
        {
            Value returned_value = stack_value_pop(&vm->stack_value);

            VirtualMachine_move_value_from_stack_to_heap(vm, current_function_call->frame_start);
            FunctionCall* returned_function_call = StackFunctionCall_pop(&vm->function_calls);
            if (StackFunctionCall_is_empty(&vm->function_calls)) {
                stack_value_pop(&vm->stack_value);
                return Interpreter_Ok;
            }

//          Note: Clears or Pops all the locals declared in the current function 
            vm->stack_value.top = returned_function_call->frame_start;
            stack_value_push(&vm->stack_value, returned_value);
            current_function_call = StackFunctionCall_peek(&vm->function_calls, 0);

#ifdef DEBUG_TRACE_EXECUTION
            ObjectString* function_name = current_function_call->closure->function->name;
            char* title = function_name == NULL ? "Script" : function_name->characters;
            printf("\n");
            Bytecode_disassemble_header(title);
#endif 

            break;
        }
        }
    }

#undef READ_BYTE_THEN_INCREMENT
#undef READ_2BYTE
#undef READ_3BYTE_THEN_INCREMENT
#undef READ_CONSTANT
#undef READ_CONSTANT_3BYTE
#undef READ_STRING
}

void VirtualMachine_runtime_error(VirtualMachine* vm, const char* format, ...) {
//  TODO: script:21:10: error: expected a funson to call
//        init():8:12: info: expected ';' before 'return'

    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    // NOTE: Prints the FunctionCalls Stack
    // 
    for (int i = 0; i < vm->function_calls.top; i++) {
        FunctionCall* function_call = StackFunctionCall_peek(&vm->function_calls, i);
        ObjectFunction* function = function_call->closure->function;
        size_t instruction = function_call->ip - function->bytecode.instructions.items - 1;
        int line = function->bytecode.lines.items[instruction];
        fprintf(stderr, "[line %d] in ", line);
        if (function->name == NULL) {
            fprintf(stderr, "script\n");
        } else {
            fprintf(stderr, "%s()\n", function->name->characters);
        }
    }

    stack_value_reset(&vm->stack_value);
}

void VirtualMachine_free(VirtualMachine* vm)
{
    Object* object = vm->objects;
    while (object != NULL) {
        Object* next = object->next;
        Object_free(object);
        object = next;
    }

    hash_table_free(&vm->global_database);
    hash_table_free(&vm->string_database);
    vm->object_init_string = NULL;
}

static void VirtualMachine_define_function_native(VirtualMachine* vm, const char* function_name, FunctionNative* function, int arity) {
    String source_string = string_make(function_name, strlen(function_name));
    uint32_t source_hash = string_hash(source_string);
    ObjectString* key    = ObjectString_Allocate(
        .task = (
            AllocateTask_Check_If_Interned |
            AllocateTask_Copy_String       |
            AllocateTask_Initialize        |
            AllocateTask_Intern
        ),
        .string = source_string,
        .hash = source_hash,
        .first = &vm->objects,
        .table = &vm->string_database
    );

    Memory_transaction_push(value_make_object(key));
    ObjectFunctionNative* value = ObjectFunctionNative_allocate(function, &vm->objects, arity);
    Memory_transaction_push(value_make_object(value));

    // stack_value_push(&vm->stack_value, value_make_object(key));
    // stack_value_push(&vm->stack_value, value_make_object(value));

    {   // Define or set a key/value pair in the global hashtable
        ObjectString* key = value_as_string(stack_value_peek(&vm->stack_value, 1));
        Value value = stack_value_peek(&vm->stack_value, 0);
        hash_table_set_value(&vm->global_database, key, value);
    }

    Memory_transaction_pop();
    Memory_transaction_pop();

    // stack_value_pop(&vm->stack_value);
    // stack_value_pop(&vm->stack_value);
}

static bool VirtualMachine_call_closure(VirtualMachine* vm, ObjectClosure* closure, int argument_count) {
    ObjectFunction* function = closure->function;
    if (argument_count != function->arity) {
        VirtualMachine_runtime_error(vm, "Expected %d arguments but got %d.", function->arity, argument_count);
        return false;
    }

    if (StackFunctionCall_is_full(&vm->function_calls)) {
        VirtualMachine_runtime_error(vm, "Function Call Stack Overflow.");
        return false;
    }

    StackFunctionCall_push(
        &vm->function_calls,
        closure,
        vm->stack_value.top,
        argument_count
    );

#ifdef DEBUG_TRACE_EXECUTION
            ObjectString* function_name = function->name;
            char* title = function_name == NULL ? "Script" : function_name->characters;
            printf("\n");
            Bytecode_disassemble_header(title);
#endif 

    return true;
}

static bool VirtualMachine_call_method(VirtualMachine* vm, ObjectString* method_name, int argument_count) {
    Value value_instance = stack_value_peek(&vm->stack_value, argument_count);
    if (!value_is_instance(value_instance)) {
        VirtualMachine_runtime_error(vm, "Only instances have methods.");
        return false;
    }

    ObjectInstance* instance = value_as_instance(value_instance);

    Value value = {0};
    if (hash_table_get_value(&instance->fields, method_name, &value)) {
        vm->stack_value.top[-argument_count - 1] = value;
        return VirtualMachine_call_value(vm, value, argument_count);
    }

    return VirtualMachine_call_from_class(vm, instance->klass, method_name, argument_count);
}

static bool VirtualMachine_call_from_class(VirtualMachine* vm, ObjectClass* klass, ObjectString* method_name, int argument_count) {
    Value closure_method = {0};
    if (!hash_table_get_value(&klass->methods, method_name, &closure_method)) {
        VirtualMachine_runtime_error(vm, "Undefined property '%s'.", method_name->characters);
        return false;
    }

    return VirtualMachine_call_closure(vm, value_as_closure(closure_method), argument_count);
}

static bool VirtualMachine_call_value(VirtualMachine* vm, Value value, int argument_count) {
    if (value_is_object(value)) {
        switch (value_get_object_type(value))
        {
        case ObjectKind_Closure: {
            return VirtualMachine_call_closure(vm, value_as_closure(value), argument_count);
        }
        case ObjectKind_Function_Native: {
            ObjectFunctionNative* function_native = value_as_function_native(value);
            if (argument_count != function_native->arity) {
                VirtualMachine_runtime_error(vm, "Expected %d arguments but got %d.", function_native->arity, argument_count);
                return false;
            }

            FunctionNative* native = function_native->function;
            Value returned_value = native(vm, argument_count, vm->stack_value.top - argument_count);
            if (value_is_runtime_error(returned_value)) return false;

            vm->stack_value.top -= argument_count + 1;
            stack_value_push(&vm->stack_value, returned_value);

            return true;
        }
        case ObjectKind_Class: {
            ObjectClass* klass = value_as_class(value);
            ObjectInstance* instance = ObjectInstance_allocate(klass, &vm->objects);
            vm->stack_value.top[-argument_count - 1] = value_make_object(instance);

            Value konstrutor_method = {0};
            if (hash_table_get_value(&klass->methods, vm->object_init_string, &konstrutor_method)) {
                return VirtualMachine_call_value(vm, konstrutor_method, argument_count);
            } 
            else if (argument_count != 0) {
                VirtualMachine_runtime_error(vm, "Expected 0 arguments but got %d.", argument_count);
                return false;
            }

            return true;
        } 
        case ObjectKind_Method: {
            ObjectMethod* obj_method = value_as_method(value);
            vm->stack_value.top[-argument_count - 1] = obj_method->instance;

            return VirtualMachine_call_closure(vm, obj_method->method, argument_count);
        }
        default: break;
        }
    }

    VirtualMachine_runtime_error(vm, "Can only call functions and classes.");
    return false;
}

static ObjectValue* VirtualMachine_create_heap_value(VirtualMachine* vm, Value* value_address) {
    ObjectValue* next = NULL;
    LinkedList_foreach(ObjectValue, vm->heap_values, object_value) {
        if (object_value.curr->value_address == value_address) {
            return object_value.curr;
        }
        if (object_value.curr->value_address < value_address) {
            next = object_value.next;
            break;
        }
    }

    ObjectValue* new_object_value = ObjectValue_allocate(&vm->objects, value_address);
    if (LinkedList_is_empty(vm->heap_values)) LinkedList_push(vm->heap_values, new_object_value);
    else if (next == NULL)                    LinkedList_push(vm->heap_values, new_object_value);
    else                                      LinkedList_insert_after(next, new_object_value);

    return new_object_value;
}

static void VirtualMachine_move_value_from_stack_to_heap(VirtualMachine* vm, Value* value_address) {
    while (vm->heap_values != NULL && vm->heap_values->value_address >= value_address) {
        ObjectValue* object_value = vm->heap_values;
        object_value->value = *object_value->value_address;
        object_value->value_address = &object_value->value;

//      Removes the current Object_Value from the Tracker from the top
        vm->heap_values = object_value->next;
    }
}

// TODO: delete code bellow
//
// static inline bool Object_check_value_kind(Value value, ObjectKind object_kind) {
//     return (value_is_object(value) && value_as_object(value)->kind == object_kind);
// }

static Value FunctionNative_clock(VirtualMachine* vm, int argument_count, Value* arguments) {
    if (argument_count > 0) {
        VirtualMachine_runtime_error(vm, "Function 'relogio' doesn't accept arguments.");
        return value_make_Runtime_Error();
    }
    return value_make_number((double)clock() / CLOCKS_PER_SEC);
}