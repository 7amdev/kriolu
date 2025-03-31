#include "kriolu.h"

//
// Globals
//

VirtualMachine g_vm;

void virtual_machine_init(VirtualMachine* vm) {
    vm->objects = NULL; // TODO: remove this??

    stack_value_reset(&vm->stack_value);
    hash_table_init(&vm->globals);
    hash_table_init(&vm->strings);
}

void virtual_machine_runtime_error(VirtualMachine* vm, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    size_t instruction = vm->ip - vm->bytecode->instructions.items - 1;
    int line = vm->bytecode->lines.items[instruction];
    fprintf(stderr, "[line %d] in script\n", line);
    stack_value_reset(&vm->stack_value);
}

InterpreterResult virtual_machine_interpret(VirtualMachine* vm, Bytecode* bytecode)
{
    vm->bytecode = bytecode;
    vm->ip = vm->bytecode->instructions.items;

    assert(vm->bytecode->instructions.items != NULL);

#define READ_BYTE_THEN_INCREMENT() (*vm->ip++)
#define READ_3BYTE_THEN_INCREMENT() (((((*vm->ip++) << 8) | (*vm->ip++)) << 8) | (*vm->ip++))
#define READ_CONSTANT() (vm->bytecode->values.items[READ_BYTE_THEN_INCREMENT()])
#define READ_CONSTANT_3BYTE() (vm->bytecode->values.items[READ_3BYTE_THEN_INCREMENT()])
#define READ_STRING() value_as_string(READ_CONSTANT())

#ifdef DEBUG_TRACE_EXECUTION
    printf("                                     Operand  \n");
    printf("Offset Line         OpCode         index value\n");
    printf("------ ---- ---------------------- ----- -----\n");
#endif

    for (;;)
    {

#ifdef DEBUG_TRACE_EXECUTION
        stack_value_trace(&vm->stack_value);
        bytecode_disassemble_instruction(
            vm->bytecode,
            (int)(vm->ip - vm->bytecode->instructions.items));
#endif

        uint8_t instruction = READ_BYTE_THEN_INCREMENT();
        switch (instruction)
        {
        default:
        {
            assert(false && "Unsupported OpCode. Handle the OpCode by adding a if statement.");
        }
        case OpCode_Constant:
        {
            Value constant = READ_CONSTANT();
            stack_value_push(&vm->stack_value, constant);
            break;
        }
        case OpCode_Constant_Long:
        {
            Value constant = READ_CONSTANT_3BYTE();
            stack_value_push(&vm->stack_value, constant);
            break;
        }
        case OpCode_True:
        {
            stack_value_push(&vm->stack_value, value_make_boolean(true));
            break;
        }
        case OpCode_False:
        {
            stack_value_push(&vm->stack_value, value_make_boolean(false));
            break;
        }
        case OpCode_Pop:
        {
            stack_value_pop(&vm->stack_value);
            break;
        }
        case OpCode_Define_Global:
        {
            ObjectString* variable_name = READ_STRING();
            Value value = stack_value_peek(&vm->stack_value, 0);
            hash_table_set_value(&vm->globals, variable_name, value);
            stack_value_pop(&vm->stack_value);
            break;
        }
        case OpCode_Nil:
        {
            stack_value_push(&vm->stack_value, value_make_nil());
            break;
        }
        case OpCode_Negation:
        {
            if (!value_is_number(stack_value_peek(&vm->stack_value, 0)))
            {
                virtual_machine_runtime_error(vm, "Operand must be a number.");
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
        case OpCode_Addition:
        {
            if (value_is_number(stack_value_peek(&vm->stack_value, 0)) &&
                value_is_number(stack_value_peek(&vm->stack_value, 1)))
            {
                Value b = stack_value_pop(&vm->stack_value);
                Value a = stack_value_pop(&vm->stack_value);
                double sum = value_as_number(a) + value_as_number(b);
                Value value_sum = value_make_number(sum);

                stack_value_push(&vm->stack_value, value_sum);
            } else if (value_is_string(stack_value_peek(&vm->stack_value, 0)) &&
                       value_is_string(stack_value_peek(&vm->stack_value, 1)))
            {
                Value b = stack_value_pop(&vm->stack_value);
                Value a = stack_value_pop(&vm->stack_value);
                ObjectString* os_a = value_as_string(a);
                ObjectString* os_b = value_as_string(b);
                String s_a = string_make(os_a->characters, os_a->length);
                String s_b = string_make(os_b->characters, os_b->length);

                String final = string_concatenate(s_a, s_b);
                uint32_t hash = string_hash(final);
                ObjectString* string = hash_table_get_key(&g_vm.strings, final, hash);
                bool found_string_in_database = (string != NULL);
                if (found_string_in_database) string_free(&final);

                string = object_create_and_intern_string(final.characters, final.length, hash);
                stack_value_push(&vm->stack_value, value_make_object(string));

                // if (string == NULL)
                //     // string = object_allocate_string(final.characters, final.length, hash);
                //     string = object_create_and_intern_string(final.characters, final.length, hash);
                // else
                //     string_free(&final);
                // stack_value_push(&vm->stack_value, value_make_object(string));
            } else
            {
                virtual_machine_runtime_error(vm, "Operands must be 2(two) numbers or 2(two) strings.");
                return Interpreter_Runtime_Error;
            }

            break;
        }
        case OpCode_Subtraction:
        {
            if (!value_is_number(stack_value_peek(&vm->stack_value, 0)) ||
                !value_is_number(stack_value_peek(&vm->stack_value, 1)))
            {
                virtual_machine_runtime_error(vm, "Operands must be numbers.");
                return Interpreter_Runtime_Error;
            }

            Value b = stack_value_pop(&vm->stack_value);
            Value a = stack_value_pop(&vm->stack_value);
            double difference = value_as_number(a) - value_as_number(b);
            Value value_difference = value_make_number(difference);

            stack_value_push(&vm->stack_value, value_difference);
            break;
        }
        case OpCode_Multiplication:
        {
            if (!value_is_number(stack_value_peek(&vm->stack_value, 0)) ||
                !value_is_number(stack_value_peek(&vm->stack_value, 1)))
            {
                virtual_machine_runtime_error(vm, "Operands must be numbers.");
                return Interpreter_Runtime_Error;
            }

            Value b = stack_value_pop(&vm->stack_value);
            Value a = stack_value_pop(&vm->stack_value);
            double product = value_as_number(a) * value_as_number(b);
            Value value_product = value_make_number(product);

            stack_value_push(&vm->stack_value, value_product);
            break;
        }
        case OpCode_Division:
        {
            if (!value_is_number(stack_value_peek(&vm->stack_value, 0)) ||
                !value_is_number(stack_value_peek(&vm->stack_value, 1)))
            {
                virtual_machine_runtime_error(vm, "Operands must be numbers.");
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
                virtual_machine_runtime_error(vm, "Operands must be numbers.");
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
                virtual_machine_runtime_error(vm, "Operands must be numbers.");
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
                virtual_machine_runtime_error(vm, "Operands must be numbers.");
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
        case OpCode_Return:
        {
            // Value constant = stack_value_pop(&vm->stack_value);
            // value_print(constant); // printf("%g", constant);
            // printf("\n");

            return Interpreter_Ok;
        }
        }
    }

#undef READ_BYTE_THEN_INCREMENT
#undef READ_3BYTE_THEN_INCREMENT
#undef READ_CONSTANT
#undef READ_CONSTANT_3BYTE
#undef READ_STRING
}

void virtual_machine_free(VirtualMachine* vm)
{
    Object* object = vm->objects;
    while (object != NULL)
    {
        Object* next = object->next;
        object_free(object);
        object = next;
    }
    hash_table_free(&vm->globals);
    hash_table_free(&vm->strings);
}