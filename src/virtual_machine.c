#include "kriolu.h"
#include <stdarg.h>

//
// Globals
//

VirtualMachine g_vm;

void virtual_machine_init(VirtualMachine *vm, Bytecode *bytecode)
{
    vm->bytecode = bytecode;
    stack_value_reset(&vm->stack_value);
    vm->ip = vm->bytecode->instructions.items;
}

void virtual_machine_runtime_error(VirtualMachine *vm, const char *format, ...)
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

InterpreterResult virtual_machine_interpret(VirtualMachine *vm)
{
    assert(vm->bytecode->instructions.items != NULL);
    assert(vm->ip == vm->bytecode->instructions.items);

#define READ_BYTE_THEN_INCREMENT() (*vm->ip++)
#define READ_3BYTE_THEN_INCREMENT() (((((*vm->ip++) << 8) | (*vm->ip++)) << 8) | (*vm->ip++))
#define READ_CONSTANT() (vm->bytecode->values.items[READ_BYTE_THEN_INCREMENT()])
#define READ_CONSTANT_3BYTE() (vm->bytecode->values.items[READ_3BYTE_THEN_INCREMENT()])

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

        // TODO: Handle instruction OpCode_True, OpCode_False, OpCode_Nil

        uint8_t instruction = READ_BYTE_THEN_INCREMENT();
        switch (instruction)
        {
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
        case OpCode_Addition:
        {
            if (!value_is_number(stack_value_peek(&vm->stack_value, 0)) ||
                !value_is_number(stack_value_peek(&vm->stack_value, 1)))
            {
                virtual_machine_runtime_error(vm, "Operands must be numbers.");
                return Interpreter_Runtime_Error;
            }

            Value b = stack_value_pop(&vm->stack_value);
            Value a = stack_value_pop(&vm->stack_value);
            double sum = value_as_number(a) + value_as_number(b);
            Value value_sum = value_make_number(sum);

            stack_value_push(&vm->stack_value, value_sum);
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
        case OpCode_Return:
        {
            Value constant = stack_value_pop(&vm->stack_value);
            value_print(constant); // printf("%g", constant);
            printf("\n");

            return Interpreter_Ok;
        }
        }
    }

#undef READ_BYTE_THEN_INCREMENT
#undef READ_3BYTE_THEN_INCREMENT
#undef READ_CONSTANT
#undef READ_CONSTANT_3BYTE
}

void virtual_machine_free(VirtualMachine *vm)
{
    assert(false && "To Be Done");
}