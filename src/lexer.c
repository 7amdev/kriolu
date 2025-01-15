#include "kriolu.h"

// TODO
// [] Scan digits
// [] Scan plus sign
// [] Scan minus sign
// [] Make comment, digit, string test files

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
static token_kind_t lexer_keyword_check(
    const char *start,
    int identifier_length,
    char const *check_text,
    int check_start_position,
    token_kind_t return_value);
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
            // length = (int)((lexer->current - 1) - (lexer->start + 2));
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
            return lexer_error(lexer, "Expeected '\"' character, instead found EOF.");
        }

        lexer_advance(lexer);

        return (token_t){
            .kind = TOKEN_STRING,
            .start = lexer->start,
            .length = (int)(lexer->current - lexer->start),
            .line_number = lexer->line_number};
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
        lexer->start = lexer->current;
        lexer_advance(lexer);

        if (*lexer->current == '=')
        {
            lexer_advance(lexer);
            return (token_t){
                .kind = TOKEN_EQUAL_EQUAL,
                .start = lexer->start,
                .length = (int)(lexer->current - lexer->start),
                .line_number = lexer->line_number};
        }

        return (token_t){
            .kind = TOKEN_EQUAL,
            .start = lexer->start,
            .length = (int)(lexer->current - lexer->start),
            .line_number = lexer->line_number};
    }

    return lexer_error(lexer, "Unexpected Caracter.");
}

void lexer_print(lexer_t *lexer)
{
    token_t token;
    for (;;)
    {
        token = lexer_scan(lexer);

        if (token.kind == TOKEN_ERROR)
        {
            fprintf(stdout, "Error: %s\n", token.start);
            break;
        }

        if (token.kind == TOKEN_EOF)
        {
            fprintf(stdout, "<EOF>");
            break;
        }

        switch (token.kind)
        {
        case TOKEN_LEFT_PARENTHESIS:
        {
            fprintf(stdout, "<LEFT_PARENTHESIS: symbol=\'(\' line=%d>\n", token.line_number);
            break;
        }
        case TOKEN_RIGHT_PARENTHESIS:
        {
            fprintf(stdout, "<RIGHT_PARENTHESIS symbol=\')\' line=%d>\n", token.line_number);
            break;
        }
        case TOKEN_LEFT_BRACE:
        {
            fprintf(stdout, "<LEFT_BRACE symbol=\'{\' line=%d>\n", token.line_number);
            break;
        }
        case TOKEN_RIGHT_BRACE:
        {
            fprintf(stdout, "<RIGHT_BRACE symbol=\'}\' line=%d>\n", token.line_number);
            break;
        }
        case TOKEN_COMMA:
        {
            fprintf(stdout, "<COMMA symbol=\',\' line=%d>\n", token.line_number);
            break;
        }
        case TOKEN_DOT:
        {
            fprintf(stdout, "<DOT symbol=\'.\' line=%d>\n", token.line_number);
            break;
        }
        case TOKEN_MINUS:
        {
            fprintf(stdout, "<MINUS symbol=\'-\' line=%d>\n", token.line_number);
            break;
        }
        case TOKEN_PLUS:
        {
            fprintf(stdout, "<PLUS symbol=\'+\' line=%d>\n", token.line_number);
            break;
        }
        case TOKEN_SLASH:
        {
            fprintf(stdout, "<SLASH symbol=\'/\' line=%d>\n", token.line_number);
            break;
        }
        case TOKEN_ASTERISK:
        {
            fprintf(stdout, "<ASTERISK symbol=\'*\' line=%d>\n", token.line_number);
            break;
        }
        case TOKEN_SEMICOLON:
        {
            fprintf(stdout, "<SEMICOLON symbol=\';\' line=%d>\n", token.line_number);
            break;
        }
        case TOKEN_NUMBER:
        {
            fprintf(stdout, "<NUMBER value=%.*s line=%d>\n", token.length, token.start, token.line_number);
            break;
        }
        case TOKEN_COMMENT:
        {
            fprintf(stdout, "<COMMENT value=%.*s line=%d>\n", token.length, token.start, token.line_number);
            break;
        }
        case TOKEN_STRING:
        {
            fprintf(stdout, "<STRING value=%.*s line=%d>\n", token.length, token.start, token.line_number);
            break;
        }
        case TOKEN_EQUAL:
        {
            fprintf(stdout, "<EQUAL symbol=\'=\' line=%d>\n", token.line_number);
            break;
        }
        case TOKEN_EQUAL_EQUAL:
        {
            fprintf(stdout, "<EQUAL_EQUAL symbol=\'==\' line=%d>\n", token.line_number);
            break;
        }
        }
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

static token_t lexer_error(lexer_t *lexer, const char *message)
{
    return (token_t){
        .kind = TOKEN_ERROR,
        .start = message,
        .length = (int)strlen(message),
        .line_number = lexer->line_number};
}