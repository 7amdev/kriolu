#include "kriolu.h"

#define LEXER_MEMORY_POOL_MAX 25

typedef struct
{
    bool allocated;
    Lexer lexer;
} LexerMemoryPool;

static LexerMemoryPool lexer_memory_pool[LEXER_MEMORY_POOL_MAX];

static char lexer_advance(Lexer *lexer);
static Token lexer_advance_then_make_token(Lexer *lexer, TokenKind kind);
static bool lexer_advance_if_match(Lexer *lexer, char expected, int offset);
static char lexer_peek(Lexer *lexer);
static Token lexer_make_token(Lexer *lexer, TokenKind kind);
static Token lexer_make_error_and_advance(Lexer *lexer, const char *format, ...);
static Token lexer_read_string(Lexer *lexer);
static bool lexer_is_eof(char c);
static bool lexer_is_digit(char c);
static bool lexer_is_letter_or_underscore(char c);
static bool lexer_is_whitespace(char c);
static bool lexer_is_comment(Lexer *lexer);
static bool lexer_is_new_line(char c);
static bool lexer_is_string(char c);
// TODO: static bool lexer_is_uppercase_letter(char c);
// TODO: static bool lexer_is_lowercase_letter(char c);
static TokenKind lexer_keyword_kind(Token token, char const *keyword, int check_start_position, TokenKind return_kind);

Lexer *lexer_create_static()
{
    Lexer *lexer = NULL;

    // TODO: make it faster using pointer arithmetic
    for (int i = 0; i < LEXER_MEMORY_POOL_MAX; ++i)
    {
        if (lexer_memory_pool[i].allocated == false)
        {
            lexer_memory_pool->allocated = true;
            lexer = &(lexer_memory_pool[i].lexer);
            break;
        }
    }

    return lexer;
}

void lexer_init(Lexer *lexer, const char *source_code)
{
    lexer->current = source_code;
    lexer->line_number = 1;
    lexer->string_interpolation_count = 0;
}

Token lexer_scan(Lexer *lexer)
{
    while (lexer_is_whitespace(*lexer->current))
    {
        if (lexer_is_new_line(*lexer->current))
        {
            lexer->line_number += 1;
        }

        lexer_advance(lexer);
    }

    lexer->start = lexer->current;

    if (lexer_is_comment(lexer))
    {
        int length;

        while (!lexer_is_new_line(*lexer->current) && !lexer_is_eof(*lexer->current))
        {
            lexer_advance(lexer);
        }

        if (lexer->current[-1] == '\r')
        {
            length = (int)((lexer->current - 1) - (lexer->start));
        }
        else
        {
            length = (int)(lexer->current - lexer->start);
        }

        return lexer_make_token(lexer, TOKEN_COMMENT);
    }

    if (lexer_is_eof(*lexer->current))
        return lexer_make_token(lexer, TOKEN_EOF);

    if (*lexer->current == '(')
        return lexer_advance_then_make_token(lexer, TOKEN_LEFT_PARENTHESIS);

    if (*lexer->current == ')')
        return lexer_advance_then_make_token(lexer, TOKEN_RIGHT_PARENTHESIS);

    if (*lexer->current == '{')
        return lexer_advance_then_make_token(lexer, TOKEN_LEFT_BRACE);

    if (*lexer->current == '}')
    {
        if (lexer->string_interpolation_count > 0)
        {
            int current_interpolation = lexer->string_interpolation_count - 1;
            lexer->string_nested_interpolation[current_interpolation] -= 1;
            if (lexer->string_nested_interpolation[current_interpolation] == 0)
            {
                lexer->string_interpolation_count -= 1;
                return lexer_read_string(lexer);
            }
        }

        return lexer_advance_then_make_token(lexer, TOKEN_RIGHT_BRACE);
    }

    if (*lexer->current == ',')
        return lexer_advance_then_make_token(lexer, TOKEN_COMMA);

    if (*lexer->current == '.')
        return lexer_advance_then_make_token(lexer, TOKEN_DOT);

    if (*lexer->current == '-')
        return lexer_advance_then_make_token(lexer, TOKEN_MINUS);

    if (*lexer->current == '+')
        return lexer_advance_then_make_token(lexer, TOKEN_PLUS);

    if (*lexer->current == '/')
        return lexer_advance_then_make_token(lexer, TOKEN_SLASH);

    if (*lexer->current == '*')
        return lexer_advance_then_make_token(lexer, TOKEN_ASTERISK);

    if (*lexer->current == '^')
        return lexer_advance_then_make_token(lexer, TOKEN_CARET);

    if (*lexer->current == ';')
        return lexer_advance_then_make_token(lexer, TOKEN_SEMICOLON);

    if (*lexer->current == '=')
    {
        Token token;
        token.start = lexer->current;
        token.line_number = lexer->line_number;

        lexer_advance(lexer);

        if (*lexer->current == '/' && lexer->current[1] == '=')
        {
            lexer_advance(lexer);
            lexer_advance(lexer);

            token.kind = TOKEN_NOT_EQUAL;
            token.length = (int)(lexer->current - lexer->start);
        }
        else if (*lexer->current == '=')
        {
            lexer_advance(lexer);
            token.kind = TOKEN_EQUAL_EQUAL;
            token.length = (int)(lexer->current - lexer->start);
        }
        else
        {
            token.kind = TOKEN_EQUAL;
            token.length = (int)(lexer->current - lexer->start);
        }

        return token;
    }

    if (*lexer->current == '>')
    {
        Token token;
        token.start = lexer->current;
        token.line_number = lexer->line_number;

        lexer_advance(lexer);

        if (*lexer->current == '=')
        {
            lexer_advance(lexer);
            token.kind = TOKEN_GREATER_EQUAL;
            token.length = (int)(lexer->current - lexer->start);
        }
        else
        {
            token.kind = TOKEN_GREATER;
            token.length = (int)(lexer->current - lexer->start);
        }

        return token;
    }

    if (*lexer->current == '<')
    {
        Token token;
        token.start = lexer->current;
        token.line_number = lexer->line_number;

        lexer_advance(lexer);

        if (*lexer->current == '=')
        {
            lexer_advance(lexer);
            token.kind = TOKEN_LESS_EQUAL;
            token.length = (int)(lexer->current - lexer->start);
        }
        else // Default
        {
            token.kind = TOKEN_LESS;
            token.length = (int)(lexer->current - lexer->start);
        }

        return token;
    }

    if (lexer_is_digit(*lexer->current))
    {
        while (lexer_is_digit(*lexer->current))
            lexer_advance(lexer);

        if (*lexer->current == '.' && lexer_is_digit(lexer->current[1]))
        {
            lexer_advance(lexer);

            while (lexer_is_digit(*lexer->current))
                lexer_advance(lexer);
        }

        return lexer_make_token(lexer, TOKEN_NUMBER);
    }

    if (lexer_is_string(*lexer->current))
        return lexer_read_string(lexer);

    if (lexer_is_letter_or_underscore(*lexer->current))
    {
        Token token;
        token.kind = TOKEN_IDENTIFIER;
        token.start = lexer->current;
        token.line_number = lexer->line_number;

        while (lexer_is_letter_or_underscore(*lexer->current) || lexer_is_digit(*lexer->current))
        {
            lexer_advance(lexer);
        }

        token.length = (int)(lexer->current - token.start);

        if (*token.start == 'e')
        {
            token.kind = lexer_keyword_kind(token, "e", 0, TOKEN_E);
        }
        else if (*token.start == 'o')
        {
            token.kind = lexer_keyword_kind(token, "ou", 1, TOKEN_OU);
        }
        else if (*token.start == 'k')
        {
            if (token.start[1] == 'a')
            {
                token.kind = lexer_keyword_kind(token, "ka", 1, TOKEN_KA);
            }
            else if (token.start[1] == 'l')
            {
                token.kind = lexer_keyword_kind(token, "klasi", 2, TOKEN_KLASI);
            }
            else if (token.start[1] == 'e')
            {
                token.kind = lexer_keyword_kind(token, "keli", 2, TOKEN_KELI);
            }
        }
        else if (*token.start == 's')
        {
            if (token.start[1] == 'i')
            {
                if (token.start[2] == 'n')
                {
                    token.kind = lexer_keyword_kind(token, "sinou", 2, TOKEN_SINOU);
                }
                else
                {
                    token.kind = lexer_keyword_kind(token, "si", 2, TOKEN_SI);
                }
            }
            else if (token.start[1] == 'u')
            {
                token.kind = lexer_keyword_kind(token, "super", 2, TOKEN_SUPER);
            }
        }
        else if (*token.start == 'f')
        {
            if (token.start[1] == 'a')
            {
                token.kind = lexer_keyword_kind(token, "falsu", 2, TOKEN_FALSU);
            }
            else if (token.start[1] == 'u')
            {
                token.kind = lexer_keyword_kind(token, "funson", 2, TOKEN_FUNSON);
            }
        }
        else if (*token.start == 'd')
        {
            if (token.start[1] == 'i')
            {
                if (token.start[2] == 'v')
                {
                    token.kind = lexer_keyword_kind(token, "divolvi", 3, TOKEN_DIVOLVI);
                }
                else
                {
                    token.kind = lexer_keyword_kind(token, "di", 2, TOKEN_DI);
                }
            }
        }
        else if (*token.start == 't')
        {
            if (token.start[1] == 'i')
            {
                if (token.start[2] == 'm')
                {
                    token.kind = lexer_keyword_kind(token, "timenti", 3, TOKEN_TIMENTI);
                }
                else
                {
                    token.kind = lexer_keyword_kind(token, "ti", 2, TOKEN_TI);
                }
            }
        }
        else if (*token.start == 'i')
        {
            token.kind = lexer_keyword_kind(token, "imprimi", 1, TOKEN_IMPRIMI);
        }
        else if (*token.start == 'v')
        {
            token.kind = lexer_keyword_kind(token, "verdadi", 1, TOKEN_VERDADI);
        }
        else if (*token.start == 'm')
        {
            token.kind = lexer_keyword_kind(token, "mimoria", 1, TOKEN_MIMORIA);
        }
        else if (*token.start == 'n')
        {
            token.kind = lexer_keyword_kind(token, "nulo", 1, TOKEN_NULO);
        }

        return token;
    }

    return lexer_make_error_and_advance(lexer, "'%c' : Lexer doesn't recognize this character.", *lexer->current);
}

void lexer_debug_print_token(Token token, const char *format)
{
    fprintf(stdout, "%2d ", token.line_number);
    switch (token.kind)
    {
    case TOKEN_ERROR:
    {
        fprintf(stdout, format, "<error>");
        break;
    }
    case TOKEN_LEFT_PARENTHESIS:
    {
        fprintf(stdout, format, "<left-parenthesis>");
        break;
    }
    case TOKEN_RIGHT_PARENTHESIS:
    {
        fprintf(stdout, format, "<right_parenthesis>");
        break;
    }
    case TOKEN_LEFT_BRACE:
    {
        fprintf(stdout, format, "<left_brace>");
        break;
    }
    case TOKEN_RIGHT_BRACE:
    {
        fprintf(stdout, format, "<right_brace>");
        break;
    }
    case TOKEN_COMMA:
    {
        fprintf(stdout, format, "<comma>");
        break;
    }
    case TOKEN_DOT:
    {
        fprintf(stdout, format, "<dot>");
        break;
    }
    case TOKEN_MINUS:
    {
        fprintf(stdout, format, "<minus>");
        break;
    }
    case TOKEN_PLUS:
    {
        fprintf(stdout, format, "<plus>");
        break;
    }
    case TOKEN_SLASH:
    {
        fprintf(stdout, format, "<slash>");
        break;
    }
    case TOKEN_ASTERISK:
    {
        fprintf(stdout, format, "<asterisk>");
        break;
    }
    case TOKEN_CARET:
    {
        fprintf(stdout, format, "<caret>");
        break;
    }
    case TOKEN_SEMICOLON:
    {
        fprintf(stdout, format, "<semicolon>");
        break;
    }
    case TOKEN_NUMBER:
    {
        fprintf(stdout, format, "<number>");
        break;
    }
    case TOKEN_COMMENT:
    {
        fprintf(stdout, format, "<comment>");
        break;
    }
    case TOKEN_STRING:
    {
        fprintf(stdout, format, "<string>");
        break;
    }
    case TOKEN_STRING_INTERPOLATION:
    {
        fprintf(stdout, format, "<string_interpolation>");
        break;
    }
    case TOKEN_EQUAL:
    {
        fprintf(stdout, format, "<equal>");
        break;
    }
    case TOKEN_EQUAL_EQUAL:
    {
        fprintf(stdout, format, "<equal_equal>");
        break;
    }
    case TOKEN_NOT_EQUAL:
    {
        fprintf(stdout, format, "<not_equal>");
        break;
    }
    case TOKEN_LESS:
    {
        fprintf(stdout, format, "<less>");
        break;
    }
    case TOKEN_LESS_EQUAL:
    {
        fprintf(stdout, format, "<less_equal>");
        break;
    }
    case TOKEN_GREATER:
    {
        fprintf(stdout, format, "<greater>");
        break;
    }
    case TOKEN_GREATER_EQUAL:
    {
        fprintf(stdout, format, "<greater_equal>");
        break;
    }
    case TOKEN_IDENTIFIER:
    {
        fprintf(stdout, format, "<identifier>");
        break;
    }
    case TOKEN_E:
    {
        fprintf(stdout, format, "<e>");
        break;
    }
    case TOKEN_OU:
    {
        fprintf(stdout, format, "<ou>");
        break;
    }
    case TOKEN_KLASI:
    {
        fprintf(stdout, format, "<klasi>");
        break;
    }
    case TOKEN_SI:
    {
        fprintf(stdout, format, "<si>");
        break;
    }
    case TOKEN_SINOU:
    {
        fprintf(stdout, format, "<sinou>");
        break;
    }
    case TOKEN_FALSU:
    {
        fprintf(stdout, format, "<falsu>");
        break;
    }
    case TOKEN_VERDADI:
    {
        fprintf(stdout, format, "<verdadi>");
        break;
    }
    case TOKEN_DI:
    {
        fprintf(stdout, format, "<di>");
        break;
    }
    case TOKEN_FUNSON:
    {
        fprintf(stdout, format, "<funson>");
        break;
    }
    case TOKEN_NULO:
    {
        fprintf(stdout, format, "<nulo>");
        break;
    }
    case TOKEN_IMPRIMI:
    {
        fprintf(stdout, format, "<imprimi>");
        break;
    }
    case TOKEN_DIVOLVI:
    {
        fprintf(stdout, format, "<divolvi>");
        break;
    }
    case TOKEN_SUPER:
    {
        fprintf(stdout, format, "<super>");
        break;
    }
    case TOKEN_KELI:
    {
        fprintf(stdout, format, "<keli>");
        break;
    }
    case TOKEN_MIMORIA:
    {
        fprintf(stdout, format, "<mimoria>");
        break;
    }
    case TOKEN_TIMENTI:
    {
        fprintf(stdout, format, "<timenti>");
        break;
    }
    case TOKEN_TI:
    {
        fprintf(stdout, format, "<ti>");
        break;
    }
    case TOKEN_KA:
    {
        fprintf(stdout, format, "<ka>");
        break;
    }
    }
    fprintf(stdout, "'%.*s' \n", token.length, token.start);
}

void lexer_debug_dump_tokens(Lexer *lexer)
{
    Token token;
    for (;;)
    {
        token = lexer_scan(lexer);

        if (token.kind == TOKEN_EOF)
            break;

        lexer_debug_print_token(token, "%-25s");
    }
}

static char lexer_advance(Lexer *lexer)
{
    lexer->current += 1;
    return lexer->current[-1];
}

static char lexer_peek(Lexer *lexer)
{
    return lexer->current[0];
}

static bool lexer_advance_if_match(Lexer *lexer, char expected, int offset)
{
    if (lexer_is_eof(lexer->current[offset]))
        return false;

    if (lexer->current[offset] != expected)
        return false;

    lexer_advance(lexer);
    return true;
}

static Token lexer_make_token(Lexer *lexer, TokenKind kind)
{
    Token token;
    token.kind = kind;
    token.start = lexer->start;
    token.length = (int)(lexer->current - lexer->start);
    token.line_number = lexer->line_number;

    return token;
}

static Token lexer_advance_then_make_token(Lexer *lexer, TokenKind kind)
{
    // Order matters!
    // First advance then make token
    //
    lexer_advance(lexer);
    Token token = lexer_make_token(lexer, kind);
    return token;
}

static Token lexer_read_string(Lexer *lexer)
{
    TokenKind token_kind = TOKEN_STRING;

    for (;;)
    {
        lexer_advance(lexer);

        if (*lexer->current == '"')
            break;

        if (lexer_is_eof(*lexer->current))
            return lexer_make_error_and_advance(lexer, "Expected '\"' character, instead found EOF.");

        if (lexer_is_new_line(*lexer->current))
            lexer->line_number += 1;

        if (*lexer->current == '%')
        {
            if (lexer->current[1] != '{')
                return lexer_make_error_and_advance(lexer, "Expected '{' character, instead found '%c'", lexer->current[1]);

            if (lexer->string_interpolation_count >= STRING_INTERPOLATION_MAX)
                return lexer_make_error_and_advance(lexer, "You've reached the maximum number of interpolation nesting.");

            token_kind = TOKEN_STRING_INTERPOLATION;
            lexer->string_nested_interpolation[lexer->string_interpolation_count] = 1;
            lexer->string_interpolation_count += 1;

            // Consume '%' character
            //
            lexer_advance(lexer);
            break;
        }
    }

    return lexer_advance_then_make_token(lexer, token_kind);
}

static bool lexer_is_eof(char c)
{
    return c == '\0';
}

static bool lexer_is_whitespace(char c)
{
    return (c == ' ') ||
           (c == '\t') ||
           (c == '\n') ||
           (c == '\r');
    //    || (c == '/');
}

static bool lexer_is_comment(Lexer *lexer)
{
    return lexer->current[0] == '/' &&
           lexer->current[1] == '/';
}

static bool lexer_is_new_line(char c)
{
    // TODO:
    // #if _windows_
    //    return lexer->curent[0] == '\r' && lexer->current[1] == '\n';
    // #elif _unix_
    //    return lexer->current[0] == '\n';
    // #elif _mac_
    //    return lexer->current[0] == '\n';
    // #endif

    return c == '\n';
}

static bool lexer_is_digit(char c)
{
    return c >= '0' && c <= '9';
}

static bool lexer_is_string(char c)
{
    return c == '"';
}

static bool lexer_is_letter_or_underscore(char c)
{
    return c == '_' ||
           (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z');
}

static TokenKind lexer_keyword_kind(Token token, char const *keyword, int check_start_position, TokenKind return_kind)
{
    int keyword_length = strlen(keyword);

    if (keyword_length != token.length)
    {
        return TOKEN_IDENTIFIER;
    }

    for (int i = check_start_position; i < keyword_length; i++)
    {
        if (token.start[i] != keyword[i])
            return TOKEN_IDENTIFIER;
    }

    return return_kind;
}

static Token lexer_make_error_and_advance(Lexer *lexer, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    char buffer[1024];
    vsnprintf(buffer, 1024, format, args);
    va_end(args);

    lexer_advance(lexer);
    return (Token){
        .kind = TOKEN_ERROR,
        .start = buffer,
        .length = (int)strlen(buffer),
        .line_number = lexer->line_number};
}

void lexer_destroy_static(Lexer *lexer)
{
    for (int i = 0; i < LEXER_MEMORY_POOL_MAX; ++i)
    {
        if (lexer == &(lexer_memory_pool[i].lexer))
        {
            assert(lexer_memory_pool[i].allocated == true);
            lexer_memory_pool[i].allocated = false;
            lexer_memory_pool[i].lexer = (Lexer){0};
            lexer = NULL;
            return;
        }
    }
    assert(false); // this is a bug, look into it.
}