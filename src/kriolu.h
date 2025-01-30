#ifndef kriolu_h
#define kriolu_h

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

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

// TODO: use tagged unions data structure to support string
//       line_number_begin and line_number_end
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
// Parser
//

typedef uint8_t AstNodeKind;
enum
{
    AST_NODE_NUMBER,
    AST_NODE_ADDITION,
    AST_NODE_SUBTRACTION,
    AST_NODE_MULTIPLICATION,
    AST_NODE_DIVISION,
    AST_NODE_EXPONENTIATION
};

typedef struct AstExpression AstExpression;
typedef struct
{
    double value;
} AstNodeNumber;

typedef struct
{
    AstExpression *left_operand;
    AstExpression *right_operand;
} AstNodeAddition;

typedef struct
{
    AstExpression *left_operand;
    AstExpression *right_operand;
} AstNodeSubtraction;

typedef struct
{
    AstExpression *left_operand;
    AstExpression *right_operand;
} AstNodeMultiplication;

typedef struct
{
    AstExpression *left_operand;
    AstExpression *right_operand;
} AstNodeDivision;

typedef struct
{
    AstExpression *left_operand;
    AstExpression *right_operand;
} AstNodeExponentiation;

struct AstExpression
{
    AstNodeKind kind;
    union
    {
        AstNodeNumber number;
        AstNodeAddition addition;
        AstNodeSubtraction subtraction;
        AstNodeMultiplication multiplication;
        AstNodeDivision division;
        AstNodeExponentiation exponentiation;
    };
};

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
void parser_parse(parser_t *parser);
void parser_advance(parser_t *parser);
void parser_consume(parser_t *parser, token_kind_t kind, const char *error_message);
bool parser_match_then_advance(parser_t *parser, token_kind_t kind);
void parser_synchronize(parser_t *parser);
void parser_error(parser_t *parser, token_t *token, const char *message);
void parser_declaration(parser_t *parser);
void parser_expression_statement(parser_t *parser);
AstExpression *parser_expression(parser_t *parser, OrderOfOperation operator_precedence_previous);
AstExpression *parser_unary_and_literals(parser);
AstExpression *parser_binary(parser_t *parser, AstExpression *left_operand);
AstExpression *parser_ast_expression_allocate();
void parser_ast_expression_free();
void parser_ast_expression_print(AstExpression *ast_node);

#endif