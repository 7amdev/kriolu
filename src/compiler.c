#include "kriolu.h"

void compiler_init(compiler_t *compiler)
{
}

int compiler_compile(compiler_t *compiler, const char *source)
{
    lexer_t lexer;
    parser_t parser;

    lexer_init(&lexer, source);
    parser_init(&parser, &lexer, false);

    parser_parse(&parser);

    return 0;
}

static void compiler_expression_statement()
{
}

static void compiler_expression()
{
}