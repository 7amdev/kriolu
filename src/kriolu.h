#ifndef kriolu_h
#define kriolu_h

//
// Token
//

typedef enum
{
    TOKEN_EOF
} token_kind_t;

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

#endif