#include "kriolu.h"

void compiler_init(compiler_t *compiler)
{
}

int compiler_compile(const char *source)
{
    lexer_t lexer;
    parser_t parser;

    lexer_init(&lexer, source);
    parser_init(&parser, &lexer, true);

    p_advance();

    while (!parser_match_then_advance(&parser, TOKEN_EOF))
    {
        l_debug_print_token(parser.current);
    }

    return 0;
}

static void compiler_expression_statement()
{
}

static void compiler_expression()
{
}