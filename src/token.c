#include "kriolu.h"

Token token_make(TokenKind kind, const char* start, int length, int line_number)
{
    Token token;

    token.kind = kind;
    token.start = start;
    token.length = length;
    token.line_number = line_number;

    return token;
}

bool token_is_identifier_equal(Token* a, Token* b) {
    // if (a->kind != Token_Identifier) return false;
    // if (b->kind != Token_Identifier) return false;
    if (a->length != b->length)      return false;
    if (a->kind != b->kind)          return false;

    return memcmp(a->start, b->start, a->length) == 0;
}