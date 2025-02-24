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
#define READ_BYTE_THEN_INCREMENT() (*vm->ip++)
#define READ_CONSTANT() (vm->bytecode->values.items[READ_BYTE_THEN_INCREMENT()])
    // todo: read_constant_long macro

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
        case OpCode_Negation:
        {
            Value value = stack_value_pop(&vm->stack_value);
            stack_value_push(&vm->stack_value, -(value));
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
#undef READ_CONSTANT
}

void virtual_machine_free(VirtualMachine *vm)
{
}