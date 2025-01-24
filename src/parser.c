#include "kriolu.h"

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
    // TODO: fix bug
    if (parser->current.kind == kind)
    {
        parser_advance(parser);
        return true;
    }

    return false;
}

void parser_consume(parser_t *parser)
{
}

void parser_synchronize(parser_t *parser)
{
}

void parser_error(parser_t *parser, token_t *token, const char *message)
{
}