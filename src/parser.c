#include "kriolu.h"

// TODO: [] parser destroy implementation

typedef enum
{
    OPERATION_MIN,

    OPERATION_ASSIGNMENT,                  // =
    OPERATION_OR,                          // or
    OPERATION_AND,                         // and
    OPERATION_EQUALITY,                    // == =/=
    OPERATION_COMPARISON,                  // < > <= >=
    OPERATION_ADDITION_AND_SUBTRACTION,    // + -
    OPERATION_MULTIPLICATION_AND_DIVISION, // * /
    OPERATION_NEGATE,                      // Unary:  -
    OPERATION_EXPONENTIATION,              // ^   ex: -2^2 = -1 * 2^2 = -4
    OPERATION_NOT,                         // Unary: ka
    OPERATION_GROUPING_CALL_AND_GET,       // . (

    OPERATION_MAX
} OrderOfOperation;

static void parser_advance(Parser* parser);
static void parser_consume(Parser* parser, TokenKind kind, const char* error_message);
static bool parser_match_then_advance(Parser* parser, TokenKind kind);
static void parser_synchronize(Parser* parser);
static void parser_error(Parser* parser, Token* token, const char* message);
static Statement parser_statement(Parser* parser);
static Statement parser_expression_statement(Parser* parser);
static Statement parser_print_instruction(Parser* parser);
static Statement parser_variabel_declaration(Parser* parser);
static Expression* parser_expression(Parser* parser, OrderOfOperation operator_precedence_previous);
static Expression* parser_unary_and_literals(parser);
static Expression* parser_binary(Parser* parser, Expression* left_operand);
static uint8_t parser_store_identifier_into_bytecode(Parser* parser, const char* start, int length);

//
// Globals
//

void parser_initialize(Parser* parser, const char* source_code, Lexer* lexer)
{
    parser->current = (Token){ 0 };  // token_error
    parser->previous = (Token){ 0 }; // token_error
    parser->panic_mode = false;
    parser->had_error = false;
    parser->lexer = lexer;
    parser->interpolation_count_nesting = 0;
    parser->interpolation_count_value_pushed = 0;
    if (lexer == NULL)
    {
        parser->lexer = lexer_create_static();
        assert(parser->lexer);

        lexer_init(parser->lexer, source_code);
    }
}

StatementArray* parser_parse(Parser* parser)
{
    StatementArray* statements = statement_array_allocate();
    parser_advance(parser);

    for (;;) {
        if (parser->current.kind == TOKEN_EOF) break;

        Statement statement = parser_statement(parser);
        statement_array_insert(statements, statement);
    }

    bytecode_emit_byte(OpCode_Return, parser->current.line_number);

    return statements;
}

static void parser_advance(Parser* parser)
{
    parser->previous = parser->current;

    for (;;)
    {
        parser->current = lexer_scan(parser->lexer);
        if (parser->current.kind == TOKEN_COMMENT)
            continue;

        if (parser->current.kind != TOKEN_ERROR)
            break;

        parser_error(parser, &parser->current, "");
    }
}

static bool parser_match_then_advance(Parser* parser, TokenKind kind)
{
    if (parser->current.kind != kind) return false;

    parser_advance(parser);
    return true;
}

static void parser_consume(Parser* parser, TokenKind kind, const char* error_message)
{
    if (parser->current.kind == kind)
    {
        parser_advance(parser);
        return;
    }

    parser_error(parser, &parser->previous, error_message);
}

static void parser_synchronize(Parser* parser)
{
    parser->panic_mode = false;

    while (parser->current.kind != TOKEN_EOF)
    {
        if (parser->previous.kind == TOKEN_SEMICOLON)
            return;

        switch (parser->current.kind)
        {
        case TOKEN_KLASI:
        case TOKEN_FUNSON:
        case TOKEN_MIMORIA:
        case TOKEN_DI:
        case TOKEN_SI:
        case TOKEN_TIMENTI:
        case TOKEN_IMPRIMI:
        case TOKEN_DIVOLVI:
            return;

        default:; // Do nothing.
        }

        parser_advance(parser);
    }
}

static void parser_error(Parser* parser, Token* token, const char* message)
{
    if (parser->panic_mode)
        return;

    parser->panic_mode = true;
    parser->had_error = true;

    fprintf(stderr, "[line %d] Error", token->line_number);
    if (token->kind == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (token->kind == TOKEN_ERROR) {
        fprintf(stderr, " at %.*s\n", token->length, token->start);
    } else {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
        fprintf(stderr, " : '%s'\n", message);
    }
}

static Statement parser_statement(Parser* parser)
{
    // DECLARATIONS: introduces new identifiers
    //   klassi, mimoria, funson
    // INSTRUCTIONS: tells the computer to perform an action
    //   imprimi, di, si, divolvi, ripiti, block
    // EXPRESSIONS: a calculation that produces a result
    //   +, -, /, *, call '(', assignment '=', objectGet '.'
    //   variableGet, literals

    Statement statement = { 0 };

    if (parser_match_then_advance(parser, TOKEN_MIMORIA)) {
        statement = parser_variabel_declaration(parser);
    } else if (parser_match_then_advance(parser, TOKEN_IMPRIMI)) {
        statement = parser_print_instruction(parser);
    } else {
        statement = parser_expression_statement(parser);
    }

    if (parser->panic_mode)
        parser_synchronize(parser);

    return statement;
}

static Statement parser_expression_statement(Parser* parser)
{
    Expression* expression = parser_expression(parser, OPERATION_ASSIGNMENT);
    parser_consume(parser, TOKEN_SEMICOLON, "Expect ';' after expression.");
    bytecode_emit_byte(OpCode_Pop, parser->previous.line_number);

    Statement statement = (Statement){
        .kind = StatementKind_Expression,
        .expression = expression };

    return statement;
}

static Statement parser_print_instruction(Parser* parser) {
    Statement statement = { 0 };

    Expression* expression = parser_expression(parser, OPERATION_ASSIGNMENT);
    parser_consume(parser, TOKEN_SEMICOLON, "Expect ';' after expression.");
    bytecode_emit_byte(OpCode_Print, parser->previous.line_number);

    statement.kind = StatementKind_Print;
    statement.expression = expression;
    return statement;
}

static Statement parser_variabel_declaration(Parser* parser) {
    Statement statement = { 0 };

    parser_consume(parser, TOKEN_IDENTIFIER, "Expect variable name.");

    uint8_t global_index = parser_store_identifier_into_bytecode(parser, parser->previous.start, parser->previous.length);
    // ObjectString* object_string = object_create_string_if_not_interned(parser->previous.start, parser->previous.length);
    // Value value_string = value_make_object_string(object_string);
    // uint32_t value_index = bytecode_insert_value(value_string);
    // uint8_t global_index = 0;
    // if (value_index > UINT8_MAX) parser_error(parser, &parser->current, "Too many constants.");
    // else global_index = value_index;

    // Check for assignment
    // 
    if (parser_match_then_advance(parser, TOKEN_EQUAL)) {
        parser_expression(parser, OPERATION_ASSIGNMENT);
    } else {
        bytecode_emit_byte(OpCode_Nil, parser->previous.line_number);
    }

    parser_consume(parser, TOKEN_SEMICOLON, "Expected ';' after expression.");

    // Define Global Varible
    //
    bytecode_emit_opcode(OpCode_Define_Global, parser->previous.line_number);
    bytecode_emit_operand_u8(global_index, parser->previous.line_number);

    return statement;
}

static uint8_t parser_store_identifier_into_bytecode(Parser* parser, const char* start, int length) {
    ObjectString* object_string = object_create_string_if_not_interned(start, length);
    Value value_string = value_make_object_string(object_string);
    int value_index = bytecode_insert_value(value_string);
    if (value_index > UINT8_MAX) {
        parser_error(parser, &parser->current, "Too many constants.");
        return 0;
    }

    return (uint8_t)value_index;
}

static OrderOfOperation parser_operator_precedence(TokenKind kind)
{
    switch (kind)
    {
    default:
        return 0;
    case TOKEN_EQUAL: return OPERATION_ASSIGNMENT;
    case TOKEN_OU:    return OPERATION_OR;
    case TOKEN_E:     return OPERATION_AND;
    case TOKEN_EQUAL_EQUAL:
    case TOKEN_NOT_EQUAL:
        return OPERATION_EQUALITY;
    case TOKEN_GREATER:
    case TOKEN_LESS:
    case TOKEN_GREATER_EQUAL:
    case TOKEN_LESS_EQUAL:
        return OPERATION_COMPARISON;
    case TOKEN_PLUS:
    case TOKEN_MINUS:
        return OPERATION_ADDITION_AND_SUBTRACTION;
    case TOKEN_ASTERISK:
    case TOKEN_SLASH:
        return OPERATION_MULTIPLICATION_AND_DIVISION;
    case TOKEN_CARET: return OPERATION_EXPONENTIATION;
    case TOKEN_KA:    return OPERATION_NOT;
    case TOKEN_DOT:
    case TOKEN_LEFT_PARENTHESIS:
        return OPERATION_GROUPING_CALL_AND_GET;
    }
}

static Expression* parser_expression(Parser* parser, OrderOfOperation operator_precedence_previous)
{
    parser_advance(parser);

    Expression* left_operand = parser_unary_and_literals(parser);
    Expression* expression = left_operand;

    TokenKind operator_kind_current = parser->current.kind;
    OrderOfOperation operator_precedence_current = parser_operator_precedence(operator_kind_current);

    while (operator_precedence_current > operator_precedence_previous)
    {
        parser_advance(parser);
        expression = parser_binary(parser, expression);

        operator_precedence_current = parser_operator_precedence(parser->current.kind);
    }

    return expression;
}

static Expression* parser_unary_and_literals(Parser* parser)
{
    // Literals
    //
    if (parser->previous.kind == TOKEN_NUMBER)
    {
        double value = strtod(parser->previous.start, NULL);
        Value number = value_make_number(value);
        Expression e_number = expression_make_number(value);

        bytecode_emit_constant(number, parser->previous.line_number);

        return expression_allocate(e_number);
    }

    if (parser->previous.kind == TOKEN_VERDADI)
    {
        Expression e_true = expression_make_boolean(true);

        bytecode_emit_byte(OpCode_True, parser->previous.line_number);
        return expression_allocate(e_true);
    }

    if (parser->previous.kind == TOKEN_FALSU)
    {
        Expression e_false = expression_make_boolean(false);

        bytecode_emit_byte(OpCode_False, parser->previous.line_number);
        return expression_allocate(e_false);
    }

    if (parser->previous.kind == TOKEN_NULO)
    {
        Expression nil = expression_make_nil();

        bytecode_emit_byte(OpCode_Nil, parser->previous.line_number);
        return expression_allocate(nil);
    }

    if (parser->previous.kind == TOKEN_STRING)
    {
        // Check if the source_string already exists in the global string
        // database in the virtual machine. If it does, reuse it, if not
        // allocate a new one and store it in the global string database for
        // future reference.
        //
        ObjectString* string = object_create_string_if_not_interned(parser->previous.start + 1, parser->previous.length - 2);

        Value v_string = value_make_object(string);
        Expression e_string = expression_make_string(string);

        bytecode_emit_constant(v_string, parser->previous.line_number);
        return expression_allocate(e_string);
    }

    if (parser->previous.kind == TOKEN_STRING_INTERPOLATION)
    {
        for (;;)
        {
            if (parser->previous.kind != TOKEN_STRING_INTERPOLATION)
                break;

            ObjectString* string = object_create_string_if_not_interned(parser->previous.start + 1, parser->previous.length - 2);
            Value v_string = value_make_object(string);
            Expression e_string = expression_make_string(string);

            // Tracks on many values have it pushed to the stack on runtime.
            //
            parser->interpolation_count_value_pushed += 1;
            bytecode_emit_constant(v_string, parser->previous.line_number);

            parser->interpolation_count_nesting += 1;
            if (parser->current.kind != TOKEN_STRING_INTERPOLATION)
                parser->interpolation_count_value_pushed += 1;
            parser_expression(parser, OPERATION_ASSIGNMENT);
            parser->interpolation_count_nesting -= 1;

            parser_advance(parser);
        }

        if (*parser->previous.start != '}')
            parser_error(parser, &parser->previous, "Missing closing '}' in interpolation template.");

        parser->interpolation_count_value_pushed += 1;
        parser_unary_and_literals(parser);

        if (parser->interpolation_count_nesting == 0)
        {
            bytecode_emit_byte(OpCode_Interpolation, parser->previous.line_number);
            bytecode_emit_byte((uint8_t)parser->interpolation_count_value_pushed, parser->previous.line_number);
        }

        // TODO: implement string interpolation Ast-Node and return it
        //
        return NULL;
    }

    // Unary
    //
    if (parser->previous.kind == TOKEN_MINUS)
    {
        Expression* expression = parser_expression(parser, OPERATION_NEGATE);
        Expression negation = expression_make_negation(expression);

        bytecode_emit_byte(OpCode_Negation, parser->previous.line_number);
        return expression_allocate(negation);
    }

    if (parser->previous.kind == TOKEN_KA)
    {
        Expression* expression = parser_expression(parser, OPERATION_NOT);
        Expression not = expression_make_not(expression);

        bytecode_emit_byte(OpCode_Not, parser->previous.line_number);
        return expression_allocate(not);
    }

    if (parser->previous.kind == TOKEN_LEFT_PARENTHESIS)
    {
        Expression* expression = parser_expression(parser, OPERATION_ASSIGNMENT);
        Expression grouping = expression_make_grouping(expression);

        parser_consume(parser, TOKEN_RIGHT_PARENTHESIS, "Expected ')' after expression.");
        return expression_allocate(grouping);
    }

    if (parser->previous.kind == TOKEN_IDENTIFIER) {
        uint8_t global_index = parser_store_identifier_into_bytecode(parser, parser->previous.start, parser->previous.length);
        bytecode_emit_opcode(OpCode_Read_Global, parser->previous.line_number);
        bytecode_emit_operand_u8(global_index, parser->previous.line_number);

        return NULL;
    }

    // TODO: log token name
    assert(false && "Unhandled token...");

    return NULL;
}

static Expression* parser_binary(Parser* parser, Expression* left_operand)
{
    TokenKind operator_kind_previous = parser->previous.kind;
    OrderOfOperation operator_precedence_previous = parser_operator_precedence(operator_kind_previous);

    Expression* right_operand = parser_expression(parser, operator_precedence_previous);

    if (operator_kind_previous == TOKEN_PLUS)
    {
        bytecode_emit_byte(OpCode_Addition, parser->previous.line_number);

        Expression addition = expression_make_addition(left_operand, right_operand);
        return expression_allocate(addition);
    }

    if (operator_kind_previous == TOKEN_MINUS)
    {
        bytecode_emit_byte(OpCode_Addition, parser->previous.line_number);

        Expression subtraction = expression_make_subtraction(left_operand, right_operand);
        return expression_allocate(subtraction);
    }

    if (operator_kind_previous == TOKEN_ASTERISK)
    {
        bytecode_emit_byte(OpCode_Multiplication, parser->previous.line_number);

        Expression multiplication = expression_make_multiplication(left_operand, right_operand);
        return expression_allocate(multiplication);
    }

    if (operator_kind_previous == TOKEN_SLASH)
    {
        bytecode_emit_byte(OpCode_Division, parser->previous.line_number);

        Expression division = expression_make_division(left_operand, right_operand);
        return expression_allocate(division);
    }

    if (operator_kind_previous == TOKEN_CARET)
    {
        bytecode_emit_byte(OpCode_Exponentiation, parser->previous.line_number);

        Expression exponentiation = expression_make_division(left_operand, right_operand);
        return expression_allocate(exponentiation);
    }

    if (operator_kind_previous == TOKEN_EQUAL_EQUAL)
    {
        bytecode_emit_byte(OpCode_Equal_To, parser->previous.line_number);

        Expression equal_to = expression_make_equal_to(left_operand, right_operand);
        return expression_allocate(equal_to);
    }

    if (operator_kind_previous == TOKEN_NOT_EQUAL)
    {
        // a != b has the same semantics as !(a == b)
        //
        bytecode_emit_byte(OpCode_Equal_To, parser->previous.line_number);
        bytecode_emit_byte(OpCode_Not, parser->previous.line_number);

        Expression* equal_to = expression_allocate(expression_make_equal_to(left_operand, right_operand));
        return expression_allocate(expression_make_not(equal_to));
    }

    if (operator_kind_previous == TOKEN_GREATER)
    {
        bytecode_emit_byte(OpCode_Greater_Than, parser->previous.line_number);

        Expression greater_than = expression_make_greater_than(left_operand, right_operand);
        return expression_allocate(greater_than);
    }

    if (operator_kind_previous == TOKEN_GREATER_EQUAL)
    {
        // a >= b has the same semantics as !(a < b)
        //
        bytecode_emit_byte(OpCode_Less_Than, parser->previous.line_number);
        bytecode_emit_byte(OpCode_Not, parser->previous.line_number);

        Expression* greater_than = expression_allocate(expression_make_greater_than(left_operand, right_operand));
        Expression greater_than_or_equal_to = expression_make_not(greater_than);
        return expression_allocate(greater_than_or_equal_to);
    }

    if (operator_kind_previous == TOKEN_LESS)
    {
        bytecode_emit_byte(OpCode_Less_Than, parser->previous.line_number);

        Expression less_than = expression_make_less_than(left_operand, right_operand);
        return expression_allocate(less_than);
    }

    if (operator_kind_previous == TOKEN_LESS_EQUAL)
    {
        // a <= b has the same semantics as !(a > b)
        //
        bytecode_emit_byte(OpCode_Greater_Than, parser->previous.line_number);
        bytecode_emit_byte(OpCode_Not, parser->previous.line_number);

        Expression* less_than = expression_allocate(expression_make_less_than(left_operand, right_operand));
        Expression less_than_or_equal_to = expression_make_not(less_than);
        return expression_allocate(less_than_or_equal_to);
    }

    // TODO: log token name
    assert(false && "Unhandled token...");

    return NULL;
}

// todo: parser destroy implementation