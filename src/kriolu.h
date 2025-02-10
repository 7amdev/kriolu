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

typedef struct
{
    const char *start;
    const char *current;
    int line_number;
} Lexer;

#define l_debug_print_token(token) lexer_debug_print_token(token, "%s ")

void lexer_init(Lexer *lexer, const char *source_code);
Token lexer_scan(Lexer *lexer);
void lexer_debug_print_token(Token token, const char *format);
void lexer_debug_dump_tokens(Lexer *lexer);

//
// Compiler
//

typedef struct
{
    Token locals[256];
    int local_count;
} Compiler;

void compiler_init(Compiler *compiler);
int compiler_compile(Compiler *compiler, const char *source);

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
    Token current;
    Token previous;
    Lexer *lexer;
    bool had_error;
    bool panic_mode;
} Parser;

extern Parser *parser_global;

#define p_advance() parser_advance(parser_global)
#define p_consume() parser_consume(parser_global)
#define p_match(...) parser_match(parser_global, __VA_ARGS__)
#define p_synchronize() parser_synchronize(parser_global)
#define p_error(...) parser_error(parser_global, __VA_ARGS__)

void parser_init(Parser *parser, Lexer *lexer, bool set_global);
StatementArray *parser_parse(Parser *parser);
void parser_advance(Parser *parser);
void parser_consume(Parser *parser, TokenKind kind, const char *error_message);
bool parser_match_then_advance(Parser *parser, TokenKind kind);
void parser_synchronize(Parser *parser);
void parser_error(Parser *parser, Token *token, const char *message);
Statement parser_statement(Parser *parser);
Statement parser_expression_statement(Parser *parser);
Expression *parser_expression(Parser *parser, OrderOfOperation operator_precedence_previous);
Expression *parser_unary_and_literals(parser);
Expression *parser_binary(Parser *parser, Expression *left_operand);
void parser_expression_print_ast(Expression *ast_node);
Expression *parser_expression_allocate_ast();
void parser_expression_free_ast(Expression *expression);

#endif