#include "kriolu.h"

static char lexer_advance(lexer_t *lexer);
static char lexer_peek(lexer_t *lexer);
static bool lexer_match(lexer_t *lexer, char expected);
static bool lexer_is_eof(char c);
static bool lexer_is_digit(char c);
static bool lexer_is_letter_or_underscore(char c);
static bool lexer_is_whitespace(char c);
static bool lexer_is_comment(lexer_t *lexer);
static bool lexer_is_new_line(char c);
static bool lexer_is_string(char c);
// TODO: static bool lexer_is_uppercase_letter(char c);
// TODO: static bool lexer_is_lowercase_letter(char c);
static token_kind_t lexer_keyword_kind(token_t token, char const *keyword, int check_start_position, token_kind_t return_kind);
static token_t lexer_error(lexer_t *lexer, const char *message);

void lexer_init(lexer_t *lexer, const char *source_code)
{
    lexer->current = source_code;
    lexer->line_number = 1;
}

token_t lexer_scan(lexer_t *lexer)
{
    while (lexer_is_whitespace(*lexer->current))
    {
        if (lexer_is_new_line(*lexer->current))
        {
            lexer->line_number += 1;
        }

        lexer_advance(lexer);
    }

    if (lexer_is_comment(lexer))
    {
        int length;

        lexer->start = lexer->current;

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

        return (token_t){
            .kind = TOKEN_COMMENT,
            .start = lexer->start,
            .length = length,
            .line_number = lexer->line_number};
    }

    if (lexer_is_eof(*lexer->current))
    {
        return (token_t){
            .kind = TOKEN_EOF,
            .start = lexer->current,
            .line_number = lexer->line_number,
            .length = 1};
    }

    if (*lexer->current == '(')
    {
        lexer->start = lexer->current;
        lexer_advance(lexer);

        return (token_t){
            .kind = TOKEN_LEFT_PARENTHESIS,
            .start = lexer->start,
            .length = (int)(lexer->current - lexer->start),
            .line_number = lexer->line_number};
    }

    if (*lexer->current == ')')
    {
        lexer->start = lexer->current;
        lexer_advance(lexer);

        return (token_t){
            .kind = TOKEN_RIGHT_PARENTHESIS,
            .start = lexer->start,
            .length = (int)(lexer->current - lexer->start),
            .line_number = lexer->line_number};
    }

    if (*lexer->current == '{')
    {
        lexer->start = lexer->current;
        lexer_advance(lexer);

        return (token_t){
            .kind = TOKEN_LEFT_BRACE,
            .start = lexer->start,
            .length = (int)(lexer->current - lexer->start),
            .line_number = lexer->line_number};
    }

    if (*lexer->current == '}')
    {
        lexer->start = lexer->current;
        lexer_advance(lexer);

        return (token_t){
            .kind = TOKEN_RIGHT_BRACE,
            .start = lexer->start,
            .length = (int)(lexer->current - lexer->start),
            .line_number = lexer->line_number};
    }

    if (*lexer->current == ',')
    {
        lexer->start = lexer->current;
        lexer_advance(lexer);

        return (token_t){
            .kind = TOKEN_COMMA,
            .start = lexer->start,
            .length = (int)(lexer->current - lexer->start),
            .line_number = lexer->line_number};
    }

    if (*lexer->current == '.')
    {
        lexer->start = lexer->current;
        lexer_advance(lexer);
        return (token_t){
            .kind = TOKEN_DOT,
            .start = lexer->start,
            .length = (int)(lexer->current - lexer->start),
            .line_number = lexer->line_number};
    }

    if (*lexer->current == '-')
    {
        lexer->start = lexer->current;
        lexer_advance(lexer);
        return (token_t){
            .kind = TOKEN_MINUS,
            .start = lexer->start,
            .length = (int)(lexer->current - lexer->start),
            .line_number = lexer->line_number};
    }

    if (*lexer->current == '+')
    {
        lexer->start = lexer->current;
        lexer_advance(lexer);
        return (token_t){
            .kind = TOKEN_PLUS,
            .start = lexer->start,
            .length = (int)(lexer->current - lexer->start),
            .line_number = lexer->line_number};
    }

    if (*lexer->current == '/')
    {
        lexer->start = lexer->current;
        lexer_advance(lexer);
        return (token_t){
            .kind = TOKEN_SLASH,
            .start = lexer->start,
            .length = (int)(lexer->current - lexer->start),
            .line_number = lexer->line_number};
    }

    if (*lexer->current == '*')
    {
        lexer->start = lexer->current;
        lexer_advance(lexer);
        return (token_t){
            .kind = TOKEN_ASTERISK,
            .start = lexer->start,
            .length = (int)(lexer->current - lexer->start),
            .line_number = lexer->line_number};
    }

    if (*lexer->current == '^')
    {
        lexer->start = lexer->current;
        lexer_advance(lexer);
        return (token_t){
            .kind = TOKEN_CARET,
            .start = lexer->start,
            .length = (int)(lexer->current - lexer->start),
            .line_number = lexer->line_number};
    }

    if (*lexer->current == ';')
    {
        lexer->start = lexer->current;
        lexer_advance(lexer);
        return (token_t){
            .kind = TOKEN_SEMICOLON,
            .start = lexer->start,
            .length = (int)(lexer->current - lexer->start),
            .line_number = lexer->line_number};
    }

    if (*lexer->current == '=')
    {
        token_t token;
        token.start = lexer->current;
        token.line_number = lexer->line_number;

        lexer->start = lexer->current;
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
        token_t token;
        token.start = lexer->current;
        token.line_number = lexer->line_number;

        lexer->start = lexer->current;
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
        token_t token;
        token.start = lexer->current;
        token.line_number = lexer->line_number;

        lexer->start = lexer->current;
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
        lexer->start = lexer->current;

        while (lexer_is_digit(*lexer->current))
            lexer_advance(lexer);

        if (*lexer->current == '.' && lexer_is_digit(lexer->current[1]))
        {
            lexer_advance(lexer);

            while (lexer_is_digit(*lexer->current))
                lexer_advance(lexer);
        }

        return (token_t){
            .kind = TOKEN_NUMBER,
            .start = lexer->start,
            .length = (int)(lexer->current - lexer->start),
            .line_number = lexer->line_number};
    }

    if (lexer_is_string(*lexer->current))
    {
        lexer->start = lexer->current;

        lexer_advance(lexer);
        while (*lexer->current != '"' && !lexer_is_eof(*lexer->current))
        {
            if (lexer_is_new_line(*lexer->current))
                lexer->line_number += 1;

            lexer_advance(lexer);
        }

        if (lexer_is_eof(*lexer->current))
        {
            return lexer_error(lexer, "Expected '\"' character, instead found EOF.");
        }

        lexer_advance(lexer);

        return (token_t){
            .kind = TOKEN_STRING,
            .start = lexer->start,
            .length = (int)(lexer->current - lexer->start),
            .line_number = lexer->line_number};
    }

    if (lexer_is_letter_or_underscore(*lexer->current))
    {
        token_t token;
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

        return token;
    }

    return lexer_error(lexer, "Unexpected Caracter.");
}

void lexer_debug_print_token(token_t token, const char *format)
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

void lexer_debug_dump_tokens(lexer_t *lexer)
{
    token_t token;
    for (;;)
    {
        token = lexer_scan(lexer);

        // if (token.kind == TOKEN_ERROR)
        // {
        //     fprintf(stdout, "Error: %s\n", token.start);
        //     continue;
        // }

        if (token.kind == TOKEN_EOF)
        {
            // fprintf(stdout, "<EOF>");
            break;
        }

        lexer_debug_print_token(token, "%-25s");
    }
}

static char lexer_advance(lexer_t *lexer)
{
    lexer->current += 1;
    return lexer->current[-1];
}

static char lexer_peek(lexer_t *lexer)
{
    return lexer->current[0];
}

static bool lexer_match(lexer_t *lexer, char expected)
{
    if (lexer_is_eof(*lexer->current))
        return false;

    if (lexer->current[0] != expected)
        return false;

    lexer_advance(lexer);
    return true;
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

static bool lexer_is_comment(lexer_t *lexer)
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

static token_kind_t lexer_keyword_kind(token_t token, char const *keyword, int check_start_position, token_kind_t return_kind)
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

static token_t lexer_error(lexer_t *lexer, const char *message)
{
    // IMPLEMENTATION 1
    // lexer_advance(lexer);
    // return (token_t){
    //     .kind = TOKEN_ERROR,
    //     .start = message,
    //     .length = (int)strlen(message),
    //     .line_number = lexer->line_number};

    // IMPLEMENTATION 2
    // TODO: review later
    // static char buffer[50];
    // sprintf(buffer, "unexpected character '%.*s' at line %d.", 1, lexer->current, lexer->line_number);
    // lexer_advance(lexer);
    // return (token_t){
    //     .kind = TOKEN_ERROR,
    //     .start = buffer,
    //     .length = (int)strlen(buffer),
    //     .line_number = lexer->line_number};

    lexer->start = lexer->current;
    lexer_advance(lexer);
    return (token_t){
        .kind = TOKEN_ERROR,
        .start = lexer->start,
        .length = (int)(lexer->current - lexer->start),
        .line_number = lexer->line_number};
}