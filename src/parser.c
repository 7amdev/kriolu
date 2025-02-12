#include "kriolu.h"

// TODO:
// [x] Change parser init function to receive source code and
//    initialize lexer.
// [] Emit bytecode instructions
// [x] Encapsulate expression allocation function in the expression.c file
// [x] Refactor parser interface public and private
// [x] Change ExpresisonAst to Expression
// [x] Add statement in StatementAstArray
// [x] Deallocate statement
// [] Write test
// [] parser destroy implementation

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

static void parser_advance(Parser *parser);
static void parser_consume(Parser *parser, TokenKind kind, const char *error_message);
static bool parser_match_then_advance(Parser *parser, TokenKind kind);
static void parser_synchronize(Parser *parser);
static void parser_error(Parser *parser, Token *token, const char *message);
static Statement parser_statement(Parser *parser);
static Statement parser_expression_statement(Parser *parser);
static Expression *parser_expression(Parser *parser, OrderOfOperation operator_precedence_previous);
static Expression *parser_unary_and_literals(parser);
static Expression *parser_binary(Parser *parser, Expression *left_operand);

void parser_initialize(Parser *parser, const char *source_code, Lexer *lexer)
{
    parser->current = (Token){0};  // token_error
    parser->previous = (Token){0}; // token_error
    parser->panic_mode = false;
    parser->had_error = false;
    parser->lexer = lexer;
    if (lexer == NULL)
    {
        parser->lexer = lexer_create_static();
        assert(parser->lexer);

        lexer_init(parser->lexer, source_code);
    }
}

StatementArray *parser_parse(Parser *parser)
{
    StatementArray *statements = statement_array_allocate();
    parser_advance(parser);

    for (;;)
    {
        if (parser->current.kind == TOKEN_EOF)
            break;

        Statement statement = parser_statement(parser);
        statement_array_insert(statements, statement);
    }

    return statements;
}

static void parser_advance(Parser *parser)
{
    parser->previous = parser->current;

    for (;;)
    {
        parser->current = lexer_scan(parser->lexer);

        // todo: skip comment token
        if (parser->current.kind != TOKEN_ERROR)
            break;

        parser_error(parser, &parser->current, "Unexpected character!");
    }
}

static bool parser_match_then_advance(Parser *parser, TokenKind kind)
{

    if (parser->current.kind == kind)
    {
        parser_advance(parser);
        return true;
    }

    return false;
}

static void parser_consume(Parser *parser, TokenKind kind, const char *error_message)
{
    if (parser->current.kind == kind)
    {
        parser_advance(parser);
        return;
    }

    parser_error(parser, &parser->previous, error_message);
}

static void parser_synchronize(Parser *parser)
{
    parser->panic_mode = false;

    while (parser->current.kind != TOKEN_EOF)
    {
        if (parser->previous.kind == TOKEN_SEMICOLON)
            return;

        switch (parser->current.kind)
        {
        case TOKEN_KLASI:
        case TOKEN_FUNSON:
        case TOKEN_MIMORIA:
        case TOKEN_DI:
        case TOKEN_SI:
        case TOKEN_TIMENTI:
        case TOKEN_IMPRIMI:
        case TOKEN_DIVOLVI:
            return;

        default:; // Do nothing.
        }

        parser_advance(parser);
    }
}

static void parser_error(Parser *parser, Token *token, const char *message)
{
    if (parser->panic_mode)
        return;

    parser->panic_mode = true;
    parser->had_error = true;

    fprintf(stderr, "[line %d] Error", token->line_number);
    if (token->kind == TOKEN_EOF)
    {
        fprintf(stderr, " at end");
    }
    else
    {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }
    fprintf(stderr, " : '%s'\n", message);
}

static Statement parser_statement(Parser *parser)
{
    // DECLARATIONS: introduces new identifiers
    //   klassi, mimoria, funson
    // INSTRUCTIONS: tells the computer to perform an action
    //   imprimi, di, si, divolvi, ripiti, block
    // EXPRESSIONS: a calculation that produces a result
    //   +, -, /, *, call '(', assignment '=', objectGet '.'
    //   variableGet, literals

    Statement expression_statement = parser_expression_statement(parser);

    if (parser->panic_mode)
        parser_synchronize(parser);

    return expression_statement;
}

static Statement parser_expression_statement(Parser *parser)
{

    Expression *expression = parser_expression(parser, OPERATION_ASSIGNMENT);
    parser_consume(parser, TOKEN_SEMICOLON, "Expect ';' after expression.");

    Statement statement = (Statement){
        .kind = StatementKind_Expression,
        .expression = expression};

    return statement;
}

static OrderOfOperation parser_operator_precedence(TokenKind kind)
{
    switch (kind)
    {
    default:
        return 0;
    case TOKEN_EQUAL:
        return OPERATION_ASSIGNMENT;
    case TOKEN_OU:
        return OPERATION_OR;
    case TOKEN_E:
        return OPERATION_AND;
    case TOKEN_EQUAL_EQUAL:
    case TOKEN_NOT_EQUAL:
        return OPERATION_EQUALITY;
    case TOKEN_GREATER:
    case TOKEN_LESS:
    case TOKEN_GREATER_EQUAL:
    case TOKEN_LESS_EQUAL:
        return OPERATION_COMPARISON;
    case TOKEN_PLUS:
    case TOKEN_MINUS:
        return OPERATION_ADDITION_AND_SUBTRACTION;
    case TOKEN_ASTERISK:
    case TOKEN_SLASH:
        return OPERATION_MULTIPLICATION_AND_DIVISION;
    case TOKEN_CARET:
        return OPERATION_EXPONENTIATION;
    case TOKEN_KA:
        return OPERATION_NOT;
    case TOKEN_DOT:
    case TOKEN_LEFT_PARENTHESIS:
        return OPERATION_GROUPING_CALL_AND_GET;
    }
}

static Expression *parser_expression(Parser *parser, OrderOfOperation operator_precedence_previous)
{
    parser_advance(parser);

    Expression *left_operand = parser_unary_and_literals(parser);
    Expression *expression = left_operand;

    TokenKind operator_kind_current = parser->current.kind;
    OrderOfOperation operator_precedence_current = parser_operator_precedence(operator_kind_current);

    while (operator_precedence_current > operator_precedence_previous)
    {
        parser_advance(parser);
        expression = parser_binary(parser, left_operand);

        operator_precedence_current = parser_operator_precedence(parser->current.kind);
    }

    return expression;
}

static Expression *parser_unary_and_literals(Parser *parser)
{
    if (parser->previous.kind == TOKEN_NUMBER)
    {
        double value = strtod(parser->previous.start, NULL);
        return expression_allocate_number(value);
    }

    if (parser->previous.kind == TOKEN_MINUS)
    {
        Expression *expression = parser_expression(parser, OPERATION_NEGATE);
        return expression_allocate_negation(expression);
    }

    if (parser->previous.kind == TOKEN_LEFT_PARENTHESIS)
    {
        Expression *expression = parser_expression(parser, OPERATION_ASSIGNMENT);
        Expression *grouping = expression_allocate_grouping(expression);

        parser_consume(parser, TOKEN_RIGHT_PARENTHESIS, "Expected ')' after expression.");
        return grouping;
    }

    return NULL;
}

static Expression *parser_binary(Parser *parser, Expression *left_operand)
{
    TokenKind operator_kind_previous = parser->previous.kind;
    OrderOfOperation operator_precedence_previous = parser_operator_precedence(operator_kind_previous);

    Expression *right_operand = parser_expression(parser, operator_precedence_previous);

    if (operator_kind_previous == TOKEN_PLUS)
        return expression_allocate_binary(ExpressionKind_Addition, left_operand, right_operand);

    if (operator_kind_previous == TOKEN_MINUS)
        return expression_allocate_binary(ExpressionKind_Subtraction, left_operand, right_operand);

    if (operator_kind_previous == TOKEN_ASTERISK)
        return expression_allocate_binary(ExpressionKind_Multiplication, left_operand, right_operand);

    if (operator_kind_previous == TOKEN_SLASH)
        return expression_allocate_binary(ExpressionKind_Division, left_operand, right_operand);

    if (operator_kind_previous == TOKEN_CARET)
        return expression_allocate_binary(ExpressionKind_Exponentiation, left_operand, right_operand);

    return NULL;
}

// todo: parser destroy implementation