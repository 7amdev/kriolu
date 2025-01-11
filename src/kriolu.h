#ifndef kriolu_h
#define kriolu_h

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

//
// Token Kind
//

typedef enum
{
    TOKEN_NUMBER,
    TOKEN_PLUS,
    TOKEN_COMMENT,
    TOKEN_ERROR,
    TOKEN_EOF
} token_kind_t;

const char *token_kind_to_string(token_kind_t kind);

//
// Token
//

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

void lexer_init(lexer_t *lexer, const char *source_code);
token_t lexer_scan(lexer_t *lexer);
void lexer_print(lexer_t *lexer);

#endif