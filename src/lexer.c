#include "kriolu.h"

// TODO
// [] Create a test folder for lexer's token generation

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

        return token_make(TOKEN_COMMENT, lexer->start, length, lexer->line_number);
        // return (token_t){
        //     .kind = TOKEN_COMMENT,
        //     .start = lexer->start,
        //     .length = length,
        //     .line_number = lexer->line_number};
    }

    if (lexer_is_eof(*lexer->current))
    {
        return token_make(TOKEN_EOF, lexer->current, 1, lexer->line_number);
        // return (token_t){
        //     .kind = TOKEN_EOF,
        //     .start = lexer->current,
        //     .line_number = lexer->line_number,
        //     .length = 1};
    }

    if (*lexer->current == '(')
    {
        lexer->start = lexer->current;
        lexer_advance(lexer);

        return token_make(
            TOKEN_LEFT_PARENTHESIS,
            lexer->start,
            (int)(lexer->current - lexer->start),
            lexer->line_number);

        // return (token_t){
        //     .kind = TOKEN_LEFT_PARENTHESIS,
        //     .start = lexer->start,
        //     .length = (int)(lexer->current - lexer->start),
        //     .line_number = lexer->line_number};
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
        token_t token;
        token.kind = TOKEN_EQUAL;
        token.start = lexer->current;
        token.length = (int)(lexer->current - lexer->start);
        token.line_number = lexer->line_number;

        lexer->start = lexer->current;
        lexer_advance(lexer);

        if (*lexer->current == '/' && lexer->current[1])
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

        return token;
    }

    if (*lexer->current == '>')
    {
        token_t token;
        token.kind = TOKEN_GREATER;
        token.start = lexer->current;
        token.length = (int)(lexer->current - lexer->start);
        token.line_number = lexer->line_number;

        lexer->start = lexer->current;
        lexer_advance(lexer);

        if (*lexer->current == '=')
        {
            lexer_advance(lexer);
            token.kind = TOKEN_GREATER_EQUAL;
            token.length = (int)(lexer->current - lexer->start);
        }

        return token;
    }

    if (*lexer->current == '<')
    {
        token_t token;
        token.kind = TOKEN_LESS;
        token.start = lexer->current;
        token.length = (int)(lexer->current - lexer->start);
        token.line_number = lexer->line_number;

        lexer->start = lexer->current;
        lexer_advance(lexer);

        if (*lexer->current == '=')
        {
            lexer_advance(lexer);
            token.kind = TOKEN_LESS_EQUAL;
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
            return lexer_error(lexer, "Expeected '\"' character, instead found EOF.");
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

        lexer->start = lexer->current;

        while (lexer_is_letter_or_underscore(*lexer->current) || lexer_is_digit(*lexer->current))
        {
            lexer_advance(lexer);
        }

        token.kind = TOKEN_IDENTIFIER;
        token.start = lexer->start;
        token.length = (int)(lexer->current - lexer->start);
        token.line_number = lexer->line_number;

        // TODO: change checks from lexer-start to token.start
        if (*lexer->start == 'e')
        {
            token.kind = lexer_keyword_kind(token, "e", 0, TOKEN_E);
        }
        else if (*lexer->start == 'o')
        {
            token.kind = lexer_keyword_kind(token, "ou", 1, TOKEN_OU);
        }
        else if (*lexer->start == 'k')
        {
            if (lexer->start[1] == 'a')
            {
                token.kind = lexer_keyword_kind(token, "ka", 1, TOKEN_KA);
            }
            else if (lexer->start[1] == 'l')
            {
                token.kind = lexer_keyword_kind(token, "klasi", 2, TOKEN_KLASI);
            }
            else if (lexer->start[1] == 'e')
            {
                token.kind = lexer_keyword_kind(token, "keli", 2, TOKEN_KELI);
            }
        }
        else if (*lexer->start == 's')
        {
            if (lexer->start[1] == 'i')
            {
                if (lexer->start[2] == 'n')
                {
                    token.kind = lexer_keyword_kind(token, "sinou", 2, TOKEN_SINOU);
                }
                else
                {
                    token.kind = lexer_keyword_kind(token, "si", 2, TOKEN_SI);
                }
            }
            else if (lexer->start[1] == 'u')
            {
                token.kind = lexer_keyword_kind(token, "super", 2, TOKEN_SUPER);
            }
        }
        else if (*lexer->start == 'f')
        {
            if (lexer->start[1] == 'a')
            {
                token.kind = lexer_keyword_kind(token, "falsu", 2, TOKEN_FALSU);
            }
            else if (lexer->start[1] == 'u')
            {
                token.kind = lexer_keyword_kind(token, "funson", 2, TOKEN_FUNSON);
            }
        }
        else if (*lexer->start == 'd')
        {
            if (lexer->start[1] == 'i')
            {
                if (lexer->start[2] == 'v')
                {
                    token.kind = lexer_keyword_kind(token, "divolvi", 3, TOKEN_DIVOLVI);
                }
                else
                {
                    token.kind = lexer_keyword_kind(token, "di", 2, TOKEN_DI);
                }
            }
        }
        else if (*lexer->start == 't')
        {
            if (lexer->start[1] == 'i')
            {
                if (lexer->start[2] == 'm')
                {
                    token.kind = lexer_keyword_kind(token, "timenti", 3, TOKEN_TIMENTI);
                }
                else
                {
                    token.kind = lexer_keyword_kind(token, "ti", 2, TOKEN_TI);
                }
            }
        }
        else if (*lexer->start == 'i')
        {
            token.kind = lexer_keyword_kind(token, "imprimi", 1, TOKEN_IMPRIMI);
        }
        else if (*lexer->start == 'v')
        {
            token.kind = lexer_keyword_kind(token, "verdadi", 1, TOKEN_VERDADI);
        }
        else if (*lexer->start == 'm')
        {
            token.kind = lexer_keyword_kind(token, "mimoria", 1, TOKEN_MIMORIA);
        }

        return token;
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
            fprintf(stdout, "<LEFT_PARENTHESIS symbol='%.*s' line=%d>\n", token.length, token.start, token.line_number);
            break;
        }
        case TOKEN_RIGHT_PARENTHESIS:
        {
            fprintf(stdout, "<RIGHT_PARENTHESIS symbol='%.*s' line=%d>\n", token.length, token.start, token.line_number);
            break;
        }
        case TOKEN_LEFT_BRACE:
        {
            fprintf(stdout, "<LEFT_BRACE symbol='%.*s' line=%d>\n", token.length, token.start, token.line_number);
            break;
        }
        case TOKEN_RIGHT_BRACE:
        {
            fprintf(stdout, "<RIGHT_BRACE symbol='%.*s' line=%d>\n", token.length, token.start, token.line_number);
            break;
        }
        case TOKEN_COMMA:
        {
            fprintf(stdout, "<COMMA symbol='%.*s' line=%d>\n", token.length, token.start, token.line_number);
            break;
        }
        case TOKEN_DOT:
        {
            fprintf(stdout, "<DOT symbol='%.*s' line=%d>\n", token.length, token.start, token.line_number);
            break;
        }
        case TOKEN_MINUS:
        {
            fprintf(stdout, "<MINUS symbol='%.*s' line=%d>\n", token.length, token.start, token.line_number);
            break;
        }
        case TOKEN_PLUS:
        {
            fprintf(stdout, "<PLUS symbol='%.*s' line=%d>\n", token.length, token.start, token.line_number);
            break;
        }
        case TOKEN_SLASH:
        {
            fprintf(stdout, "<SLASH symbol='%.*s' line=%d>\n", token.length, token.start, token.line_number);
            break;
        }
        case TOKEN_ASTERISK:
        {
            fprintf(stdout, "<ASTERISK symbol='%.*s' line=%d>\n", token.length, token.start, token.line_number);
            break;
        }
        case TOKEN_SEMICOLON:
        {
            fprintf(stdout, "<SEMICOLON symbol='%.*s' line=%d>\n", token.length, token.start, token.line_number);
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
            fprintf(stdout, "<EQUAL symbol='%.*s' line=%d>\n", token.length, token.start, token.line_number);
            break;
        }
        case TOKEN_EQUAL_EQUAL:
        {
            fprintf(stdout, "<EQUAL_EQUAL symbol='%.*s' line=%d>\n", token.length, token.start, token.line_number);
            break;
        }
        case TOKEN_NOT_EQUAL:
        {
            fprintf(stdout, "<NOT_EQUAL symbol='%.*s' line=%d>\n", token.length, token.start, token.line_number);
            break;
        }
        case TOKEN_LESS:
        {
            fprintf(stdout, "<LESS symbol=\'<\' line=%d>\n", token.line_number);
            break;
        }
        case TOKEN_LESS_EQUAL:
        {
            fprintf(stdout, "<LESS_EQUAL symbol=\'>=\' line=%d>\n", token.line_number);
            break;
        }
        case TOKEN_GREATER:
        {
            fprintf(stdout, "<GREATER symbol=\'>\' line=%d>\n", token.line_number);
            break;
        }
        case TOKEN_GREATER_EQUAL:
        {
            fprintf(stdout, "<GREATER_EQUAL symbol=\'>=\' line=%d>\n", token.line_number);
            break;
        }
        case TOKEN_IDENTIFIER:
        {
            fprintf(stdout, "<IDENTIFIER value=\'%.*s\' line=%d>\n", token.length, token.start, token.line_number);
            break;
        }
        case TOKEN_E:
        {
            fprintf(stdout, "<E value=\'%.*s\' line=%d>\n", token.length, token.start, token.line_number);
            break;
        }
        case TOKEN_OU:
        {
            fprintf(stdout, "<OU value=\'%.*s\' line=%d>\n", token.length, token.start, token.line_number);
            break;
        }
        case TOKEN_KLASI:
        {
            fprintf(stdout, "<KLASI value=\'%.*s\' line=%d>\n", token.length, token.start, token.line_number);
            break;
        }
        case TOKEN_SI:
        {
            fprintf(stdout, "<SI value=\'%.*s\' line=%d>\n", token.length, token.start, token.line_number);
            break;
        }
        case TOKEN_SINOU:
        {
            fprintf(stdout, "<SINOU value=\'%.*s\' line=%d>\n", token.length, token.start, token.line_number);
            break;
        }
        case TOKEN_FALSU:
        {
            fprintf(stdout, "<FALSU value=\'%.*s\' line=%d>\n", token.length, token.start, token.line_number);
            break;
        }
        case TOKEN_VERDADI:
        {
            fprintf(stdout, "<VERDADI value=\'%.*s\' line=%d>\n", token.length, token.start, token.line_number);
            break;
        }
        case TOKEN_DI:
        {
            fprintf(stdout, "<DI value=\'%.*s\' line=%d>\n", token.length, token.start, token.line_number);
            break;
        }
        case TOKEN_FUNSON:
        {
            fprintf(stdout, "<FUNSON value=\'%.*s\' line=%d>\n", token.length, token.start, token.line_number);
            break;
        }
        case TOKEN_NULO:
        {
            fprintf(stdout, "<NULO value=\'%.*s\' line=%d>\n", token.length, token.start, token.line_number);
            break;
        }
        case TOKEN_IMPRIMI:
        {
            fprintf(stdout, "<IMPRIMI value=\'%.*s\' line=%d>\n", token.length, token.start, token.line_number);
            break;
        }
        case TOKEN_DIVOLVI:
        {
            fprintf(stdout, "<DIVOLVI value=\'%.*s\' line=%d>\n", token.length, token.start, token.line_number);
            break;
        }
        case TOKEN_SUPER:
        {
            fprintf(stdout, "<SUPER value=\'%.*s\' line=%d>\n", token.length, token.start, token.line_number);
            break;
        }
        case TOKEN_KELI:
        {
            fprintf(stdout, "<KELI value=\'%.*s\' line=%d>\n", token.length, token.start, token.line_number);
            break;
        }
        case TOKEN_MIMORIA:
        {
            fprintf(stdout, "<MIMORIA value=\'%.*s\' line=%d>\n", token.length, token.start, token.line_number);
            break;
        }
        case TOKEN_TIMENTI:
        {
            fprintf(stdout, "<TIMENTI value=\'%.*s\' line=%d>\n", token.length, token.start, token.line_number);
            break;
        }
        case TOKEN_TI:
        {
            fprintf(stdout, "<TI value=\'%.*s\' line=%d>\n", token.length, token.start, token.line_number);
            break;
        }
        case TOKEN_KA:
        {
            fprintf(stdout, "<KA value=\'%.*s\' line=%d>\n", token.length, token.start, token.line_number);
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
    return (token_t){
        .kind = TOKEN_ERROR,
        .start = message,
        .length = (int)strlen(message),
        .line_number = lexer->line_number};
}