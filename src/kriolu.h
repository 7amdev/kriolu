#ifndef kriolu_h
#define kriolu_h

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#define DEBUG_LOG_PARSER

// todo: change type declaration name to CamelCase

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
} token_kind_t;

typedef struct
{
    token_kind_t kind;
    const char *start;
    int length;
    int line_number;
} token_t;

//
// Lexer
//

typedef struct
{
    const char *start;
    const char *current;
    int line_number;
} lexer_t;

#define l_debug_print_token(token) lexer_debug_print_token(token, "%s ")

void lexer_init(lexer_t *lexer, const char *source_code);
token_t lexer_scan(lexer_t *lexer);
void lexer_debug_print_token(token_t token, const char *format);
void lexer_debug_dump_tokens(lexer_t *lexer);

//
// Compiler
//

typedef struct
{
    token_t locals[256];
    int local_count;
} compiler_t;

void compiler_init(compiler_t *compiler);
int compiler_compile(compiler_t *compiler, const char *source);

//
// Abstract Syntax Tree
//

typedef struct Expression ExpressionAst;

typedef struct
{
    double value;
} ExpressionNumber;

typedef struct
{
    ExpressionAst *operand;
} ExpressionNegation;

typedef struct
{
    ExpressionAst *operand;
} ExpressionGrouping;

typedef struct
{
    ExpressionAst *left_operand;
    ExpressionAst *right_operand;
} ExpressionAddition;

typedef struct
{
    ExpressionAst *left_operand;
    ExpressionAst *right_operand;
} ExpressionSubtraction;

typedef struct
{
    ExpressionAst *left_operand;
    ExpressionAst *right_operand;
} ExpressionMultiplication;

typedef struct
{
    ExpressionAst *left_operand;
    ExpressionAst *right_operand;
} ExpressionDivision;

typedef struct
{
    ExpressionAst *left_operand;
    ExpressionAst *right_operand;
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
        ExpressionAst *expression;
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
void statement_array_remove_last(StatementArray *statements);
void statement_array_remove_at(StatementArray *statements, uint32_t index);
void statement_array_free(StatementArray *statements);

//
// Parser
//

typedef enum
{
    OPERATION_MIN,

    OPERATION_ASSIGNMENT,                  // =
    OPERATION_OR,                          // or
    OPERATION_AND,                         // and
    OPERATION_EQUALITY,                    // == =/=
    OPERATION_COMPARISON,                  // < > <= >=
    OPERATION_ADDITION_AND_SUBTRACTION,    // + -
    OPERATION_MULTIPLICATION_AND_DIVISION, // * /
    OPERATION_NEGATE,                      // Unary:  -
    OPERATION_EXPONENTIATION,              // ^   ex: -2^2 = -1 * 2^2 = -4
    OPERATION_NOT,                         // Unary: ka
    OPERATION_GROUPING_CALL_AND_GET,       // . (

    OPERATION_MAX
} OrderOfOperation;

typedef struct
{
    token_t current;
    token_t previous;
    lexer_t *lexer;
    bool had_error;
    bool panic_mode;
} parser_t;

extern parser_t *parser_global;

#define p_advance() parser_advance(parser_global)
#define p_consume() parser_consume(parser_global)
#define p_match(...) parser_match(parser_global, __VA_ARGS__)
#define p_synchronize() parser_synchronize(parser_global)
#define p_error(...) parser_error(parser_global, __VA_ARGS__)

void parser_init(parser_t *parser, lexer_t *lexer, bool set_global);
StatementArray *parser_parse(parser_t *parser);
void parser_advance(parser_t *parser);
void parser_consume(parser_t *parser, token_kind_t kind, const char *error_message);
bool parser_match_then_advance(parser_t *parser, token_kind_t kind);
void parser_synchronize(parser_t *parser);
void parser_error(parser_t *parser, token_t *token, const char *message);
Statement parser_statement(parser_t *parser);
Statement parser_expression_statement(parser_t *parser);
ExpressionAst *parser_expression(parser_t *parser, OrderOfOperation operator_precedence_previous);
ExpressionAst *parser_unary_and_literals(parser);
ExpressionAst *parser_binary(parser_t *parser, ExpressionAst *left_operand);
void parser_expression_print_ast(ExpressionAst *ast_node);
ExpressionAst *parser_expression_allocate_ast();
void parser_expression_free_ast();

#endif