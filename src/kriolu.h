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
// Abstract Syntax Tree
//
//
// AST Nodes Kind
//     ExpressionNumber
//     ExpresionNegation
//     ExpressoinAddition
//     ExpressionSubtraction
//     ExpressionMultiplication
//     ...

typedef struct Expression Expression;

typedef struct
{
    double value;
} ExpressionNumber;

typedef struct
{
    Expression *operand;
} ExpressionNegation;

typedef struct
{
    Expression *expression;
} ExpressionGrouping;

typedef struct
{
    Expression *left_operand;
    Expression *right_operand;
} ExpressionAddition;

typedef struct
{
    Expression *left_operand;
    Expression *right_operand;
} ExpressionSubtraction;

typedef struct
{
    Expression *left_operand;
    Expression *right_operand;
} ExpressionMultiplication;

typedef struct
{
    Expression *left_operand;
    Expression *right_operand;
} ExpressionDivision;

typedef struct
{
    Expression *left_operand;
    Expression *right_operand;
} ExpressionExponentiation;

typedef uint8_t ExpressionKind;
enum
{
    ExpressionKind_Invalid,

    ExpressionKind_Number,
    ExpressionKind_Negation,
    ExpressionKind_Grouping,
    ExpressionKind_Addition,
    ExpressionKind_Subtraction,
    ExpressionKind_Multiplication,
    ExpressionKind_Division,
    ExpressionKind_Exponentiation,
};

// TODO: make it more compact
struct Expression
{
    ExpressionKind kind;
    union
    {
        ExpressionNumber number;
        ExpressionNegation negation;
        ExpressionGrouping grouping;
        ExpressionAddition addition;
        ExpressionSubtraction subtraction;
        ExpressionMultiplication multiplication;
        ExpressionDivision division;
        ExpressionExponentiation exponentiation;
    };
};

Expression *expression_allocate(Expression expr);
Expression *expression_allocate_number(double value);
Expression *expression_allocate_grouping(Expression *expr);
Expression *expression_allocate_negation(Expression *operand);
Expression *expression_allocate_binary(ExpressionKind kind, Expression *left_operand, Expression *right_operand);
void expression_print(Expression *expression);
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
    OpCode_Addition,
    OpCode_Subtraction,
    OpCode_Multiplication,
    OpCode_Division,
    OpCode_Exponentiation,
    OpCode_Negation,

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
// Value
//

typedef double Value;

typedef struct
{
    Value *items;
    int count;
    int capacity;
} ValueArray;

void value_array_init(ValueArray *values);
uint32_t value_array_insert(ValueArray *values, Value value);
void value_array_free(ValueArray *values);

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