#include "kriolu.h"

#define LEXER_MEMORY_POOL_MAX 25

typedef struct
{
    bool allocated;
    Lexer lexer;
} LexerMemoryPool;

static LexerMemoryPool lexer_memory_pool[LEXER_MEMORY_POOL_MAX];

static char lexer_advance(Lexer* lexer);
static Token lexer_advance_then_make_token(Lexer* lexer, TokenKind kind);
static bool lexer_advance_if_match(Lexer* lexer, char expected, int offset);
static char lexer_peek(Lexer* lexer);
static Token lexer_make_token(Lexer* lexer, TokenKind kind);
static Token lexer_make_error_and_advance(Lexer* lexer, const char* format, ...);
static Token lexer_read_string(Lexer* lexer);
static bool lexer_is_eof(char c);
static bool lexer_is_digit(char c);
static bool lexer_is_letter_or_underscore(char c);
static bool lexer_is_whitespace(char c);
static bool lexer_is_comment(Lexer* lexer);
static bool lexer_is_new_line(char c);
static bool lexer_is_string(char c);
// TODO: static bool lexer_is_uppercase_letter(char c);
// TODO: static bool lexer_is_lowercase_letter(char c);
static TokenKind lexer_keyword_kind(Token token, char const* keyword, int check_start_position, TokenKind return_kind);

Lexer* lexer_create_static()
{
    Lexer* lexer = NULL;

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

void lexer_init(Lexer* lexer, const char* source_code)
{
    lexer->current = source_code;
    lexer->line_number = 1;
    lexer->string_interpolation_count = 0;
}

Token lexer_scan(Lexer* lexer)
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
        } else
        {
            length = (int)(lexer->current - lexer->start);
        }

        return lexer_make_token(lexer, Token_Comment);
    }

    if (lexer_is_eof(*lexer->current))
        return lexer_make_token(lexer, Token_Eof);

    if (*lexer->current == '(')
        return lexer_advance_then_make_token(lexer, Token_Left_Parenthesis);

    if (*lexer->current == ')')
        return lexer_advance_then_make_token(lexer, Token_Right_Parenthesis);

    if (*lexer->current == '{')
        return lexer_advance_then_make_token(lexer, Token_Left_Brace);

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

        return lexer_advance_then_make_token(lexer, Token_Right_Brace);
    }

    if (*lexer->current == ',')
        return lexer_advance_then_make_token(lexer, Token_Comma);

    if (*lexer->current == '.')
        return lexer_advance_then_make_token(lexer, Token_Dot);

    if (*lexer->current == '-')
        return lexer_advance_then_make_token(lexer, Token_Minus);

    if (*lexer->current == '+')
        return lexer_advance_then_make_token(lexer, Token_Plus);

    if (*lexer->current == '/')
        return lexer_advance_then_make_token(lexer, Token_Slash);

    if (*lexer->current == '*')
        return lexer_advance_then_make_token(lexer, Token_Asterisk);

    if (*lexer->current == '^')
        return lexer_advance_then_make_token(lexer, Token_Caret);

    if (*lexer->current == ';')
        return lexer_advance_then_make_token(lexer, Token_Semicolon);

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

            token.kind = Token_Not_Equal;
            token.length = (int)(lexer->current - lexer->start);
        } else if (*lexer->current == '=')
        {
            lexer_advance(lexer);
            token.kind = Token_Equal_Equal;
            token.length = (int)(lexer->current - lexer->start);
        } else
        {
            token.kind = Token_Equal;
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
            token.kind = Token_Greater_Equal;
            token.length = (int)(lexer->current - lexer->start);
        } else
        {
            token.kind = Token_Greater;
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
            token.kind = Token_Less_Equal;
            token.length = (int)(lexer->current - lexer->start);
        } else // Default
        {
            token.kind = Token_Less;
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

        return lexer_make_token(lexer, Token_Number);
    }

    if (lexer_is_string(*lexer->current))
        return lexer_read_string(lexer);

    if (lexer_is_letter_or_underscore(*lexer->current))
    {
        Token token;
        token.kind = Token_Identifier;
        token.start = lexer->current;
        token.line_number = lexer->line_number;

        while (lexer_is_letter_or_underscore(*lexer->current) || lexer_is_digit(*lexer->current))
        {
            lexer_advance(lexer);
        }

        token.length = (int)(lexer->current - token.start);

        if (*token.start == 'e') {
            token.kind = lexer_keyword_kind(token, "e", 0, Token_E);
        } else if (*token.start == 'o') {
            token.kind = lexer_keyword_kind(token, "ou", 1, Token_Ou);
        } else if (*token.start == 'k') {
            if (token.start[1] == 'a') {
                token.kind = lexer_keyword_kind(token, "ka", 1, Token_Ka);
            } else if (token.start[1] == 'l') {
                token.kind = lexer_keyword_kind(token, "klasi", 2, Token_Klasi);
            } else if (token.start[1] == 'e') {
                token.kind = lexer_keyword_kind(token, "keli", 2, Token_Keli);
            }
        } else if (*token.start == 's') {
            if (token.start[1] == 'i') {
                if (token.start[2] == 'n') {
                    token.kind = lexer_keyword_kind(token, "sinou", 2, Token_Sinou);
                } else {
                    token.kind = lexer_keyword_kind(token, "si", 2, Token_Si);
                }
            } else if (token.start[1] == 'u') {
                token.kind = lexer_keyword_kind(token, "super", 2, Token_Super);
            }
        } else if (*token.start == 'f') {
            if (token.start[1] == 'a') {
                token.kind = lexer_keyword_kind(token, "falsu", 2, Token_Falsu);
            } else if (token.start[1] == 'u') {
                token.kind = lexer_keyword_kind(token, "funson", 2, Token_Funson);
            }
        } else if (*token.start == 'd') {
            if (token.start[1] == 'i') {
                if (token.start[2] == 'v') {
                    token.kind = lexer_keyword_kind(token, "divolvi", 3, Token_Divolvi);
                } else {
                    token.kind = lexer_keyword_kind(token, "di", 2, Token_Di);
                }
            }
        } else if (*token.start == 't') {
            if (token.start[1] == 'i') {
                if (token.start[2] == 'm') {
                    token.kind = lexer_keyword_kind(token, "timenti", 3, Token_Timenti);
                } else {
                    token.kind = lexer_keyword_kind(token, "ti", 2, Token_Ti);
                }
            }
        } else if (*token.start == 'i') {
            token.kind = lexer_keyword_kind(token, "imprimi", 1, Token_Imprimi);
        } else if (*token.start == 'v') {
            token.kind = lexer_keyword_kind(token, "verdadi", 1, Token_Verdadi);
        } else if (*token.start == 'm') {
            token.kind = lexer_keyword_kind(token, "mimoria", 1, Token_Mimoria);
        } else if (*token.start == 'n') {
            token.kind = lexer_keyword_kind(token, "nulo", 1, Token_Nulo);
        } else if (*token.start == 'p') {
            token.kind = lexer_keyword_kind(token, "pa", 1, Token_Pa);
        }

        return token;
    }

    return lexer_make_error_and_advance(lexer, "'%c' : Lexer doesn't recognize this character.", *lexer->current);
}

void lexer_debug_print_token(Token token, const char* format)
{
    fprintf(stdout, "%2d ", token.line_number);
    switch (token.kind)
    {
    case Token_Error:
    {
        fprintf(stdout, format, "<error>");
        break;
    }
    case Token_Left_Parenthesis:
    {
        fprintf(stdout, format, "<left-parenthesis>");
        break;
    }
    case Token_Right_Parenthesis:
    {
        fprintf(stdout, format, "<right_parenthesis>");
        break;
    }
    case Token_Left_Brace:
    {
        fprintf(stdout, format, "<left_brace>");
        break;
    }
    case Token_Right_Brace:
    {
        fprintf(stdout, format, "<right_brace>");
        break;
    }
    case Token_Comma:
    {
        fprintf(stdout, format, "<comma>");
        break;
    }
    case Token_Dot:
    {
        fprintf(stdout, format, "<dot>");
        break;
    }
    case Token_Minus:
    {
        fprintf(stdout, format, "<minus>");
        break;
    }
    case Token_Plus:
    {
        fprintf(stdout, format, "<plus>");
        break;
    }
    case Token_Slash:
    {
        fprintf(stdout, format, "<slash>");
        break;
    }
    case Token_Asterisk:
    {
        fprintf(stdout, format, "<asterisk>");
        break;
    }
    case Token_Caret:
    {
        fprintf(stdout, format, "<caret>");
        break;
    }
    case Token_Semicolon:
    {
        fprintf(stdout, format, "<semicolon>");
        break;
    }
    case Token_Number:
    {
        fprintf(stdout, format, "<number>");
        break;
    }
    case Token_Comment:
    {
        fprintf(stdout, format, "<comment>");
        break;
    }
    case Token_String:
    {
        fprintf(stdout, format, "<string>");
        break;
    }
    case Token_String_Interpolation:
    {
        fprintf(stdout, format, "<string_interpolation>");
        break;
    }
    case Token_Equal:
    {
        fprintf(stdout, format, "<equal>");
        break;
    }
    case Token_Equal_Equal:
    {
        fprintf(stdout, format, "<equal_equal>");
        break;
    }
    case Token_Not_Equal:
    {
        fprintf(stdout, format, "<not_equal>");
        break;
    }
    case Token_Less:
    {
        fprintf(stdout, format, "<less>");
        break;
    }
    case Token_Less_Equal:
    {
        fprintf(stdout, format, "<less_equal>");
        break;
    }
    case Token_Greater:
    {
        fprintf(stdout, format, "<greater>");
        break;
    }
    case Token_Greater_Equal:
    {
        fprintf(stdout, format, "<greater_equal>");
        break;
    }
    case Token_Identifier:
    {
        fprintf(stdout, format, "<identifier>");
        break;
    }
    case Token_E:
    {
        fprintf(stdout, format, "<e>");
        break;
    }
    case Token_Ou:
    {
        fprintf(stdout, format, "<ou>");
        break;
    }
    case Token_Klasi:
    {
        fprintf(stdout, format, "<klasi>");
        break;
    }
    case Token_Si:
    {
        fprintf(stdout, format, "<si>");
        break;
    }
    case Token_Sinou:
    {
        fprintf(stdout, format, "<sinou>");
        break;
    }
    case Token_Falsu:
    {
        fprintf(stdout, format, "<falsu>");
        break;
    }
    case Token_Verdadi:
    {
        fprintf(stdout, format, "<verdadi>");
        break;
    }
    case Token_Di:
    {
        fprintf(stdout, format, "<di>");
        break;
    }
    case Token_Funson:
    {
        fprintf(stdout, format, "<funson>");
        break;
    }
    case Token_Nulo:
    {
        fprintf(stdout, format, "<nulo>");
        break;
    }
    case Token_Imprimi:
    {
        fprintf(stdout, format, "<imprimi>");
        break;
    }
    case Token_Divolvi:
    {
        fprintf(stdout, format, "<divolvi>");
        break;
    }
    case Token_Super:
    {
        fprintf(stdout, format, "<super>");
        break;
    }
    case Token_Keli:
    {
        fprintf(stdout, format, "<keli>");
        break;
    }
    case Token_Mimoria:
    {
        fprintf(stdout, format, "<mimoria>");
        break;
    }
    case Token_Timenti:
    {
        fprintf(stdout, format, "<timenti>");
        break;
    }
    case Token_Ti:
    {
        fprintf(stdout, format, "<ti>");
        break;
    }
    case Token_Ka:
    {
        fprintf(stdout, format, "<ka>");
        break;
    }
    }
    fprintf(stdout, "'%.*s' \n", token.length, token.start);
}

void lexer_debug_dump_tokens(Lexer* lexer)
{
    Token token;
    for (;;)
    {
        token = lexer_scan(lexer);

        if (token.kind == Token_Eof)
            break;

        lexer_debug_print_token(token, "%-25s");
    }
}

static char lexer_advance(Lexer* lexer)
{
    lexer->current += 1;
    return lexer->current[-1];
}

static char lexer_peek(Lexer* lexer)
{
    return lexer->current[0];
}

static bool lexer_advance_if_match(Lexer* lexer, char expected, int offset)
{
    if (lexer_is_eof(lexer->current[offset]))
        return false;

    if (lexer->current[offset] != expected)
        return false;

    lexer_advance(lexer);
    return true;
}

static Token lexer_make_token(Lexer* lexer, TokenKind kind)
{
    Token token;
    token.kind = kind;
    token.start = lexer->start;
    token.length = (int)(lexer->current - lexer->start);
    token.line_number = lexer->line_number;

    return token;
}

static Token lexer_advance_then_make_token(Lexer* lexer, TokenKind kind)
{
    // Order matters!
    // First advance then make token
    //
    lexer_advance(lexer);
    Token token = lexer_make_token(lexer, kind);
    return token;
}

static Token lexer_read_string(Lexer* lexer)
{
    TokenKind token_kind = Token_String;

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

            token_kind = Token_String_Interpolation;
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

static bool lexer_is_comment(Lexer* lexer)
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

static TokenKind lexer_keyword_kind(Token token, char const* keyword, int check_start_position, TokenKind return_kind)
{
    int keyword_length = strlen(keyword);

    if (keyword_length != token.length) {
        return Token_Identifier;
    }

    for (int i = check_start_position; i < keyword_length; i++) {
        if (token.start[i] != keyword[i])
            return Token_Identifier;
    }

    return return_kind;
}

static Token lexer_make_error_and_advance(Lexer* lexer, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    char buffer[1024];
    vsnprintf(buffer, 1024, format, args);
    va_end(args);

    lexer_advance(lexer);
    return (Token) {
        .kind = Token_Error,
            .start = buffer,
            .length = (int)strlen(buffer),
            .line_number = lexer->line_number
    };
}

void lexer_destroy_static(Lexer* lexer)
{
    for (int i = 0; i < LEXER_MEMORY_POOL_MAX; ++i)
    {
        if (lexer == &(lexer_memory_pool[i].lexer))
        {
            assert(lexer_memory_pool[i].allocated == true);
            lexer_memory_pool[i].allocated = false;
            lexer_memory_pool[i].lexer = (Lexer){ 0 };
            lexer = NULL;
            return;
        }
    }
    assert(false); // this is a bug, look into it.
}