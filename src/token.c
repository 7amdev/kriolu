#include "kriolu.h"

token_t token_make(token_kind_t kind, const char *start, int length, int line_number)
{
    token_t token;

    token.kind = kind;
    token.start = start;
    token.length = length;
    token.line_number = line_number;

    return token;
}