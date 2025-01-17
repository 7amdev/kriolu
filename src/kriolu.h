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
    TOKEN_LEFT_PARENTHESIS,  // (
    TOKEN_RIGHT_PARENTHESIS, // )
    TOKEN_LEFT_BRACE,        // {
    TOKEN_RIGHT_BRACE,       // }
    TOKEN_COMMA,
    TOKEN_DOT,
    TOKEN_MINUS,
    TOKEN_PLUS,
    TOKEN_SEMICOLON,
    TOKEN_SLASH,
    TOKEN_ASTERISK,

    TOKEN_EQUAL,         // =
    TOKEN_EQUAL_EQUAL,   // ==
    TOKEN_NOT_EQUAL,     // =/=
    TOKEN_GREATER,       // >
    TOKEN_GREATER_EQUAL, // >=
    TOKEN_LESS,          // <
    TOKEN_LESS_EQUAL,    // <=

    TOKEN_IDENTIFIER,
    TOKEN_STRING,
    TOKEN_NUMBER,

    TOKEN_E, // and
    TOKEN_OU,
    TOKEN_KA, // Logic NOT Operator
    TOKEN_KLASI,
    TOKEN_SI,
    TOKEN_SINOU,
    TOKEN_FALSU,
    TOKEN_VERDADI,
    TOKEN_DI,
    TOKEN_FUNSON,
    TOKEN_NULO,
    TOKEN_IMPRIMI,
    TOKEN_DIVOLVI,
    TOKEN_SUPER,
    TOKEN_KELI,    // this
    TOKEN_MIMORIA, // variable
    TOKEN_TIMENTI,
    TOKEN_TI,

    TOKEN_COMMENT,
    TOKEN_ERROR,
    TOKEN_EOF
} token_kind_t;

//
// Token
//

// TODO: use tagged unions data structure to support string
//       line_number_begin and line_number_end
typedef struct
{
    token_kind_t kind;
    const char *start;
    int length;
    int line_number;
} token_t;

token_t token_make(token_kind_t kind, const char *start, int length, int line_number);

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