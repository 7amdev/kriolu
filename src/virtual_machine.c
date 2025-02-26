#include "kriolu.h"

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
            Value value = stack_value_pop(&vm->stack_value);
            stack_value_push(&vm->stack_value, -(value));
            break;
        }
        case OpCode_Addition:
        {
            Value b = stack_value_pop(&vm->stack_value);
            Value a = stack_value_pop(&vm->stack_value);
            stack_value_push(&vm->stack_value, (a + b));

            break;
        }
        case OpCode_Subtraction:
        {
            Value b = stack_value_pop(&vm->stack_value);
            Value a = stack_value_pop(&vm->stack_value);
            stack_value_push(&vm->stack_value, (a - b));

            break;
        }
        case OpCode_Multiplication:
        {
            Value b = stack_value_pop(&vm->stack_value);
            Value a = stack_value_pop(&vm->stack_value);
            stack_value_push(&vm->stack_value, (a * b));

            break;
        }
        case OpCode_Division:
        {
            Value b = stack_value_pop(&vm->stack_value);
            Value a = stack_value_pop(&vm->stack_value);
            stack_value_push(&vm->stack_value, (a / b));

            break;
        }
        case OpCode_Exponentiation:
        {
            Value b = stack_value_pop(&vm->stack_value);
            Value a = stack_value_pop(&vm->stack_value);
            stack_value_push(&vm->stack_value, pow(a, b));

            break;
        }
        case OpCode_Return:
        {
            Value constant = stack_value_pop(&vm->stack_value);
            printf("%g", constant);
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