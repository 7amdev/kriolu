#ifndef kriolu_h
#define kriolu_h

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <math.h>

#define DEBUG_LOG_PARSER

// todo: change Array, Stack, and Ast naming convention to:
//       AstExpression, StackValue, ArrayInstruction, ArrayStatement, ...

//
// Token
//

typedef enum
{
    TOKEN_ERROR,
    TOKEN_EOF,

    TOKEN_LEFT_PARENTHESIS,  // (
    TOKEN_RIGHT_PARENTHESIS, // )
    TOKEN_LEFT_BRACE,        // {
    TOKEN_RIGHT_BRACE,       // }
    TOKEN_COMMA,
    TOKEN_DOT,
    TOKEN_MINUS,
    TOKEN_PLUS,
    TOKEN_SEMICOLON,
    TOKEN_SLASH,
    TOKEN_ASTERISK,
    TOKEN_CARET,

    TOKEN_EQUAL,         // =
    TOKEN_EQUAL_EQUAL,   // ==
    TOKEN_NOT_EQUAL,     // =/=
    TOKEN_GREATER,       // >
    TOKEN_GREATER_EQUAL, // >=
    TOKEN_LESS,          // <
    TOKEN_LESS_EQUAL,    // <=

    TOKEN_IDENTIFIER,
    TOKEN_STRING,
    TOKEN_STRING_INTERPOLATION,
    TOKEN_NUMBER,

    TOKEN_E,  // logic operator AND
    TOKEN_OU, // logic operator OR
    TOKEN_KA, // logic operator NOT
    TOKEN_KLASI,
    TOKEN_SI,
    TOKEN_SINOU,
    TOKEN_FALSU,
    TOKEN_VERDADI,
    TOKEN_DI,
    TOKEN_FUNSON,
    TOKEN_NULO,
    TOKEN_IMPRIMI,
    TOKEN_DIVOLVI,
    TOKEN_SUPER,
    TOKEN_KELI,    // this
    TOKEN_MIMORIA, // variable
    TOKEN_TIMENTI,
    TOKEN_TI,

    TOKEN_COMMENT
} TokenKind;

typedef struct
{
    TokenKind kind;
    const char *start;
    int length;
    int line_number;
} Token;

//
// Lexer
//

#define STRING_INTERPOLATION_MAX 8

typedef struct
{
    const char *start;
    const char *current;
    int line_number;

    int string_nested_interpolation[STRING_INTERPOLATION_MAX];
    int string_interpolation_count;

} Lexer;

#define l_debug_print_token(token) lexer_debug_print_token(token, "%s ")

Lexer *lexer_create_static();
void lexer_init(Lexer *lexer, const char *source_code);
Token lexer_scan(Lexer *lexer);
void lexer_debug_print_token(Token token, const char *format);
void lexer_debug_dump_tokens(Lexer *lexer);
void lexer_destroy_static(Lexer *lexer);

//
// Value
//

typedef enum
{
    Value_Boolean,
    Value_Nil,
    Value_Number
} ValueKind;

typedef struct
{
    ValueKind kind;
    union
    {
        bool boolean;
        double number;
    } as;
} Value;

typedef struct
{
    Value *items;
    int count;
    int capacity;
} ValueArray;

#define value_make_boolean(value) ((Value){.kind = Value_Boolean, .as = {.boolean = value}})
#define value_make_number(value) ((Value){.kind = Value_Number, .as = {.number = value}})
#define value_make_nil() ((Value){.kind = Value_Nil, .as = {.number = 0}})

#define value_as_boolean(value) ((value).as.boolean)
#define value_as_number(value) ((value).as.number)

#define value_is_boolean(value) ((value).kind == Value_Boolean)
#define value_is_number(value) ((value).kind == Value_Number)
#define value_is_nil(value) ((value).kind == Value_Nil)

bool value_negate_logically(Value value);

void value_array_init(ValueArray *values);
uint32_t value_array_insert(ValueArray *values, Value value);
void value_print(Value value);
void value_array_free(ValueArray *values);

//
// Abstract Syntax Tree
//

typedef uint8_t ExpressionKind;
enum
{
    ExpressionKind_Invalid,

    ExpressionKind_Number,
    ExpressionKind_Boolean,
    ExpressionKind_Nil,
    ExpressionKind_Negation,
    ExpressionKind_Not,
    ExpressionKind_Grouping,
    ExpressionKind_Addition,
    ExpressionKind_Subtraction,
    ExpressionKind_Multiplication,
    ExpressionKind_Division,
    ExpressionKind_Exponentiation,
};

typedef struct Expression Expression;
struct Expression
{
    ExpressionKind kind;
    union
    {
        double number;
        bool boolean;
        struct
        {
            Expression *operand;
        } unary;
        struct
        {
            Expression *left;
            Expression *right;
        } binary;
    } as;
};

#define expression_make_number(value) ((Expression){.kind = ExpressionKind_Number, .as = {.number = (value)}})
#define expression_make_boolean(value) ((Expression){.kind = ExpressionKind_Boolean, .as = {.boolean = (value)}})
#define expression_make_nil() ((Expression){.kind = ExpressionKind_Nil, .as = {.number = 0}})
#define expression_make_negation(expression) ((Expression){.kind = ExpressionKind_Negation, .as = {.unary = {.operand = (expression)}}})
#define expression_make_not(expression) ((Expression){.kind = ExpressionKind_Not, .as = {.unary = {.operand = (expression)}}})
#define expression_make_grouping(expression) ((Expression){.kind = ExpressionKind_Grouping, .as = {.unary = {.operand = (expression)}}})
#define expression_make_addition(left_operand, right_operand) ((Expression){.kind = ExpressionKind_Addition, .as = {.binary = {.left = (left_operand), .right = (right_operand)}}})
#define expression_make_subtraction(left_operand, right_operand) ((Expression){.kind = ExpressionKind_Subtraction, .as = {.binary = {.left = (left_operand), .right = (right_operand)}}})
#define expression_make_multiplication(left_operand, right_operand) ((Expression){.kind = ExpressionKind_Multiplication, .as = {.binary = {.left = (left_operand), .right = (right_operand)}}})
#define expression_make_division(left_operand, right_operand) ((Expression){.kind = ExpressionKind_Division, .as = {.binary = {.left = (left_operand), .right = (right_operand)}}})
#define expression_make_exponentiation(left_operand, right_operand) ((Expression){.kind = ExpressionKind_Exponentiation, .as = {.binary = {.left = (left_operand), .right = (right_operand)}}})

#define expression_as_number(expression) ((expression).as.number)
#define expression_as_boolean(expression) ((expression).as.boolean)
#define expression_as_value(expression) ((expression).as.value)
#define expression_as_negation(expression) ((expression).as.unary)
#define expression_as_not(expression) ((expression).as.unary)
#define expression_as_grouping(expression) ((expression).as.unary)
#define expression_as_binary(expression) ((expression).as.binary)
#define expression_as_addition(expression) ((expression).as.binary)
#define expression_as_subtraction(expression) ((expression).as.binary)
#define expression_as_multiplication(expression) ((expression).as.binary)
#define expression_as_division(expression) ((expression).as.binary)
#define expression_as_exponentiation(expression) ((expression).as.binary)

Expression *expression_allocate(Expression expr);
void expression_print(Expression *expression);
void expression_print_tree(Expression *expression, int indent);
void expression_free(Expression *expression);

typedef uint8_t StatementKind;
enum
{
    StatementKind_Invalid,

    StatementKind_Expression,
    StatementKind_Block,
    StatementKind_Return,
    StatementKind_Si,
    StatementKind_Di,
};

typedef struct Statement Statement;
struct Statement
{
    StatementKind kind;
    union
    {
        Expression *expression;
    };
};

typedef struct
{
    Statement *items;
    uint32_t count;
    uint32_t capacity;
} StatementArray;

StatementArray *statement_array_allocate();
uint32_t statement_array_insert(StatementArray *statements, Statement statement);
void statement_array_free(StatementArray *statements);

//
// Parser
//

typedef struct
{
    Token current;
    Token previous;
    Lexer *lexer;
    bool had_error;
    bool panic_mode;
} Parser;

#define parser_init(parser, source_code) parser_initialize(parser, source_code, NULL)

void parser_initialize(Parser *parser, const char *source_code, Lexer *lexer);
StatementArray *parser_parse(Parser *parser);

//
// Line Number
//

typedef struct
{
    int *items;
    int count;
    int capacity;
} LineNumberArray;

void line_array_init(LineNumberArray *lines);
int line_array_insert(LineNumberArray *lines, int line);
int line_array_insert_3x(LineNumberArray *lines, int line);
void line_array_free(LineNumberArray *lines);

//
// Instruction
//

typedef uint8_t OpCode;
enum
{
    OpCode_Invalid,

    OpCode_Constant,
    OpCode_Constant_Long,
    OpCode_Nil,
    OpCode_True,
    OpCode_False,
    OpCode_Negation,
    OpCode_Not,
    OpCode_Addition,
    OpCode_Subtraction,
    OpCode_Multiplication,
    OpCode_Division,
    OpCode_Exponentiation,

    OpCode_Return
};

typedef struct
{
    uint8_t *items;
    int count;
    int capacity;
} InstructionArray;

void instruction_array_init(InstructionArray *instructions);
int instruction_array_insert(InstructionArray *instructions, uint8_t item);
int instruction_array_insert_u24(InstructionArray *instructions, uint8_t byte1, uint8_t byte2, uint8_t byte3);
void instruction_array_free(InstructionArray *instructions);

//
// Bytecode
//

// Bytecode is a series of instructions. Eventually, we'll store
// some other data along with instructions, so ...
//
typedef struct
{
    InstructionArray instructions;
    LineNumberArray lines;
    ValueArray values;
} Bytecode;

extern Bytecode g_bytecode;
#define bytecode_emit_byte(data, line) bytecode_write_byte(&g_bytecode, data, line)
#define bytecode_emit_constant(value, line) bytecode_write_constant(&g_bytecode, value, line)

#define bytecode_write_opcode(bytecode, opcode, line_number) bytecode_write_byte(bytecode, opcode, line_number)
#define bytecode_write_operand_u8(bytecode, operand, line_number) bytecode_write_byte(bytecode, operand, line_number)
#define bytecode_write_operand_u24(bytecode, byte1, byte2, byte3, line_number) bytecode_write_instruction_u24(bytecode, byte1, byte2, byte3, line_number)

void bytecode_init(Bytecode *bytecode);
int bytecode_write_byte(Bytecode *bytecode, uint8_t data, int line_number);
int bytecode_write_constant(Bytecode *bytecode, Value value, int line_number);
void bytecode_disassemble(Bytecode *bytecode, const char *name);
int bytecode_disassemble_instruction(Bytecode *bytecode, int offset);
void bytecode_emitter_begin();
Bytecode bytecode_emitter_end();
void bytecode_free(Bytecode *bytecode);

//
// Value Stack
//

#define STACK_MAX 256

typedef struct
{
    Value items[STACK_MAX];
    Value *top;
} StackValue;

StackValue *stack_value_create(void);
void stack_value_reset(StackValue *stack);
Value stack_value_push(StackValue *stack, Value value);
Value stack_value_pop(StackValue *stack);
Value stack_value_peek(StackValue *stack, int offset);
bool stack_value_is_full(StackValue *stack);
bool stack_value_is_empty(StackValue *stack);
void stack_value_trace(StackValue *stack);
void stack_free(StackValue *stack);

//
// Virtual Machine
//

#define DEBUG_TRACE_EXECUTION

typedef enum
{
    Interpreter_Ok,
    Interpreter_Compiler_error,
    Interpreter_Runtime_Error
} InterpreterResult;

typedef struct
{
    Bytecode *bytecode;
    StackValue stack_value;
    uint8_t *ip; // Instruction Pointer
} VirtualMachine;

extern VirtualMachine g_vm;

#define vm_init(bytecode) virtual_machine_init(&g_vm, bytecode)
#define vm_interpret() virtual_machine_interpret(&g_vm)
#define vm_free() virtual_machine_init(&g_vm)

void virtual_machine_init(VirtualMachine *vm, Bytecode *bytecode);
InterpreterResult virtual_machine_interpret(VirtualMachine *vm);
void virtual_machine_free(VirtualMachine *vm);

#endif