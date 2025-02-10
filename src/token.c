#include "kriolu.h"

Token token_make(TokenKind kind, const char *start, int length, int line_number)
{
    Token token;

    token.kind = kind;
    token.start = start;
    token.length = length;
    token.line_number = line_number;

    return token;
}