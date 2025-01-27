#include "kriolu.h"

OrderOfOperation operator_precedence_map[] = {
    [TOKEN_NUMBER] = OPERATION_MIN,
    [TOKEN_EQUAL] = OPERATION_ASSIGNMENT,
    [TOKEN_OU] = OPERATION_OR,
    [TOKEN_E] = OPERATION_AND,
    [TOKEN_LESS] = OPERATION_COMPARISON,
    [TOKEN_LESS_EQUAL] = OPERATION_COMPARISON,
    [TOKEN_GREATER] = OPERATION_COMPARISON,
    [TOKEN_GREATER_EQUAL] = OPERATION_COMPARISON,
    [TOKEN_PLUS] = OPERATION_ADDITION_AND_SUBTRACTION,
    [TOKEN_MINUS] = OPERATION_ADDITION_AND_SUBTRACTION,
    [TOKEN_ASTERISK] = OPERATION_MULTIPLICATION_AND_DIVISION,
    [TOKEN_SLASH] = OPERATION_MULTIPLICATION_AND_DIVISION,
    [TOKEN_CARET] = OPERATION_EXPONENTIATION,
    [TOKEN_LEFT_PARENTHESIS] = OPERATION_PARENTHESIS_AND_GET,
    [TOKEN_DOT] = OPERATION_PARENTHESIS_AND_GET,
    [TOKEN_EQUAL_EQUAL] = OPERATION_EQUALITY,
    [TOKEN_NOT_EQUAL] = OPERATION_EQUALITY};

parser_t *parser_global;
void parser_init(parser_t *parser, lexer_t *lexer, bool set_global)
{
    // TODO: review current and previous initialization.
    //       token{0} is equivalent to left-parenthesis
    parser->current = (token_t){0};
    parser->previous = (token_t){0};
    parser->lexer = lexer;
    parser->panic_mode = false;
    parser->had_error = false;

    if (set_global)
    {
        parser_global = parser;
    }
}

void parser_parse(parser_t *parser)
{
    parser_advance(parser);

    for (;;)
    {
        if (parser->current.kind == TOKEN_EOF)
            break;

        parser_declaration(parser);

        // l_debug_print_token(parser->current);
    }
}

void parser_advance(parser_t *parser)
{
    parser->previous = parser->current;

    for (;;)
    {
        parser->current = lexer_scan(parser->lexer);

        // skip comment token

        if (parser->current.kind != TOKEN_ERROR)
            break;

        // log error and set parser's panicMode and hasError to true
    }
}

bool parser_match_then_advance(parser_t *parser, token_kind_t kind)
{

    if (parser->current.kind == kind)
    {
        parser_advance(parser);
        return true;
    }

    return false;
}

void parser_consume(parser_t *parser, token_kind_t kind, const char *error_message)
{
}

void parser_synchronize(parser_t *parser)
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

void parser_error(parser_t *parser, token_t *token, const char *message)
{
}

void parser_declaration(parser_t *parser)
{
    // for now
    parser_expression_statement(parser);

    // todo: test parser_synchronize
    if (parser->panic_mode)
        parser_synchronize(parser);
}

void parser_expression_statement(parser_t *parser)
{
    parser_expression(parser, OPERATION_ASSIGNMENT);
    parser_consume(parser, TOKEN_SEMICOLON, "Expect ';' after expression.");
}

void parser_expression(parser_t *parser, OrderOfOperation operator_precedence_previous)
{
    parser_advance(parser);
    parser_unary_and_literals(parser);

    token_kind_t operator_kind_current = parser->current.kind;
    OrderOfOperation operator_precedence_current = operator_precedence_map[operator_kind_current];

    while (operator_precedence_current > operator_precedence_previous)
    {
        parser_advance(parser);
        parser_binary(parser);

        operator_precedence_current = operator_precedence_map[parser->current.kind];
    }
}
void parser_unary_and_literals(parser_t *parser)
{
    if (parser->previous.kind == TOKEN_NUMBER)
    {
        double value = strtod(parser->previous.start, NULL);
        printf("%.1f\n", value);
    }
    else if (parser->previous.kind = TOKEN_MINUS)
    {
        // todo
    }
}

void parser_binary(parser_t *parser)
{
    token_kind_t operator_kind_previous = parser->previous.kind;
    OrderOfOperation operator_precedence_previous = operator_precedence_map[operator_kind_previous];

    parser_expression(parser, (OrderOfOperation)operator_precedence_previous);

    if (operator_kind_previous == TOKEN_PLUS)
    {
        printf("+\n");
    }
    else if (operator_kind_previous == TOKEN_MINUS)
    {
        printf("-\n");
    }
    else if (operator_kind_previous == TOKEN_ASTERISK)
    {
        printf("*\n");
    }
    else if (operator_kind_previous == TOKEN_SLASH)
    {
        printf("/\n");
    }
    else if (operator_kind_previous == TOKEN_CARET)
    {
        printf("^\n");
    }
}