#include "kriolu.h"

void compiler_init(Compiler *compiler)
{
}

int compiler_compile(Compiler *compiler, const char *source)
{
    Lexer lexer;
    Parser parser;

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