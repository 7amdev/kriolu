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
static Statement parser_instruction_print(Parser* parser);
static Statement parser_instruction_block(Parser* parser);
static Statement parser_instruction_if(Parser* parser);
static Statement parser_variable_declaration(Parser* parser);
static Expression* parser_expression(Parser* parser, OrderOfOperation operator_precedence_previous);
static Expression* parser_unary_and_literals(parser);
static Expression* parser_binary(Parser* parser, Expression* left_operand);
static uint8_t parser_store_identifier_into_bytecode(Parser* parser, const char* start, int length);
static bool parser_check_locals_duplicates(Parser* parser, Token* identifier);
static void parser_end(Parser* parser);
static void parser_begin_scope(Parser* parser);
static void parser_end_scope(Parser* parser);

//
// Globals
//

void parser_initialize(Parser* parser, const char* source_code, Lexer* lexer)
{
    parser->token_current = (Token){ 0 };  // token_error
    parser->token_previous = (Token){ 0 }; // token_error
    parser->panic_mode = false;
    parser->had_error = false;
    parser->lexer = lexer;
    parser->scope_current = NULL;
    parser->interpolation_count_nesting = 0;
    parser->interpolation_count_value_pushed = 0;
    if (lexer == NULL) {
        parser->lexer = lexer_create_static();
        assert(parser->lexer);
        lexer_init(parser->lexer, source_code);
    }
}

ArrayStatement* parser_parse(Parser* parser)
{
    ArrayStatement* statements = array_statement_allocate();
    Scope scope_script;
    scope_init(&scope_script);

    parser->scope_current = &scope_script;
    parser_advance(parser);

    for (;;) {
        if (parser->token_current.kind == Token_Eof) break;

        Statement statement = parser_statement(parser);
        array_statement_insert(statements, statement);
    }

    parser_end(parser);

    return statements;
}

static void parser_advance(Parser* parser)
{
    parser->token_previous = parser->token_current;

    for (;;)
    {
        parser->token_current = lexer_scan(parser->lexer);
        if (parser->token_current.kind == Token_Comment)
            continue;

        if (parser->token_current.kind != Token_Error)
            break;

        parser_error(parser, &parser->token_current, "");
    }
}

static bool parser_match_then_advance(Parser* parser, TokenKind kind)
{
    if (parser->token_current.kind != kind) return false;

    parser_advance(parser);
    return true;
}

static void parser_consume(Parser* parser, TokenKind kind, const char* error_message)
{
    if (parser->token_current.kind == kind)
    {
        parser_advance(parser);
        return;
    }

    parser_error(parser, &parser->token_previous, error_message);
}

static void parser_synchronize(Parser* parser)
{
    parser->panic_mode = false;

    while (parser->token_current.kind != Token_Eof)
    {
        if (parser->token_previous.kind == Token_Semicolon)
            return;

        switch (parser->token_current.kind)
        {
        case Token_Klasi:
        case Token_Funson:
        case Token_Mimoria:
        case Token_Di:
        case Token_Si:
        case Token_Timenti:
        case Token_Imprimi:
        case Token_Divolvi:
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
    if (token->kind == Token_Eof) {
        fprintf(stderr, " at end");
    } else if (token->kind == Token_Error) {
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

    if (parser_match_then_advance(parser, Token_Mimoria)) {
        statement = parser_variable_declaration(parser);
    } else if (parser_match_then_advance(parser, Token_Imprimi)) {
        statement = parser_instruction_print(parser);
    } else if (parser_match_then_advance(parser, Token_Si)) {
        statement = parser_instruction_if(parser);
    } else if (parser_match_then_advance(parser, Token_Left_Brace)) {
        parser_begin_scope(parser);
        parser_instruction_block(parser);
        parser_end_scope(parser);
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
    parser_consume(parser, Token_Semicolon, "Expect ';' after expression.");
    bytecode_emit_instruction_1byte(OpCode_Pop, parser->token_previous.line_number);

    Statement statement = (Statement){
        .kind = StatementKind_Expression,
        .expression = expression };

    return statement;
}

static Statement parser_instruction_print(Parser* parser) {
    Statement statement = { 0 };

    Expression* expression = parser_expression(parser, OPERATION_ASSIGNMENT);
    parser_consume(parser, Token_Semicolon, "Expect ';' after expression.");
    bytecode_emit_instruction_1byte(OpCode_Print, parser->token_previous.line_number);

    statement.kind = StatementKind_Print;
    statement.expression = expression;
    return statement;
}

static Statement parser_instruction_block(Parser* parser) {
    Statement statement = { 0 };

    for (;;) {
        TokenKind kind = parser->token_current.kind;
        if (kind == Token_Right_Brace) break;
        if (kind == Token_Eof)         break;

        parser_statement(parser);
    }

    parser_consume(parser, Token_Right_Brace, "Expect '}' after block.");

    return statement;
}

// if (condition) {then-block} sinou {else-block} 
//
static Statement parser_instruction_if(Parser* parser) {
    Statement statement = { 0 };

    parser_consume(parser, Token_Left_Parenthesis, "Expect '(' after 'if'.");
    parser_expression(parser, OPERATION_ASSIGNMENT); // Will leave a Boolean value on top of the Stack
    parser_consume(parser, Token_Right_Parenthesis, "Expect ')' after condition.");

    // If the 'if-condition' is false, jump to the begining of 'else-block'.
    //
    int jump_if_false_operand_index = bytecode_emit_instruction_jump(OpCode_Jump_If_False, parser->token_previous.line_number);

    //
    // Then-Block
    //

    // Removes the condition's value from the stack, since a statement leaves NO value in the
    // stack.
    //
    bytecode_emit_instruction_1byte(OpCode_Pop, parser->token_previous.line_number);

    // Compile then-block statements 
    //
    parser_statement(parser);

    // After finishing compiling statements in the then-block, jump out off 
    // the if-statement skipping the else-block statements.
    //
    int jump_operand_index = bytecode_emit_instruction_jump(OpCode_Jump, parser->token_previous.line_number);

    // Patches the instruction 'jump_if_false' operand with the starting else-block index.
    //
    bool patch_error = Bytecode_PatchInstructionJump(jump_if_false_operand_index);
    if (patch_error) parser_error(parser, &parser->token_previous, "Too much code to jump over.");

    //
    // Else-Block
    //

    // Removes the condition's value from the stack, since a statement leaves NO value in the
    // stack.
    //
    bytecode_emit_instruction_1byte(OpCode_Pop, parser->token_previous.line_number);

    if (parser_match_then_advance(parser, Token_Sinou)) parser_statement(parser);

    bool error = Bytecode_PatchInstructionJump(jump_operand_index);
    if (error) parser_error(parser, &parser->token_previous, "Too much code to jump over.");

    return statement;
}

static Statement parser_variable_declaration(Parser* parser) {
    Statement statement = { 0 };
    uint8_t global_index = 0;

    parser_consume(parser, Token_Identifier, "Expect variable name.");

    block{
        if (parser->scope_current->depth == 0)
            break;

        if (StackLocal_is_full(&parser->scope_current->locals)) {
            parser_error(parser, &parser->token_previous, "Too many local variables in a scope.");
            break;
        }

        if (parser_check_locals_duplicates(parser, &parser->token_previous)) {
            parser_error(parser, &parser->token_previous, "Already a variable with this name in this scope.");
            return statement;
        }

        StackLocal_push(
            &parser->scope_current->locals,
            parser->token_previous,
            -1 // Mark Local as Uninitialized 
        );

        // Check for assignment
        // 
        if (parser_match_then_advance(parser, Token_Equal)) {
            parser_expression(parser, OPERATION_ASSIGNMENT);
        } else {
            bytecode_emit_instruction_1byte(OpCode_Nil, parser->token_previous.line_number);
        }

        parser_consume(parser, Token_Semicolon, "Expected ';' after expression.");

        // Mark local as Initialized
        //
        Local* local = &parser->scope_current->locals.items[parser->scope_current->locals.count - 1];
        local->scope_depth = parser->scope_current->depth;

        return statement;
    }

        // Global Scope Only
        //
    global_index = parser_store_identifier_into_bytecode(parser, parser->token_previous.start, parser->token_previous.length);

    // Check for assignment
    // 
    if (parser_match_then_advance(parser, Token_Equal)) {
        parser_expression(parser, OPERATION_ASSIGNMENT);
    } else {
        bytecode_emit_instruction_1byte(OpCode_Nil, parser->token_previous.line_number);
    }

    parser_consume(parser, Token_Semicolon, "Expected ';' after expression.");

    // Define Global Varible
    //
    bytecode_emit_instruction_2bytes(
        OpCode_Define_Global,
        global_index,
        parser->token_previous.line_number
    );

    return statement;
}

static uint8_t parser_store_identifier_into_bytecode(Parser* parser, const char* start, int length) {
    ObjectString* object_string = ObjectString_AllocateIfNotInterned(start, length);
    Value value_string = value_make_object_string(object_string);
    int value_index = bytecode_emit_value(value_string);
    if (value_index > UINT8_MAX) {
        parser_error(parser, &parser->token_current, "Too many constants.");
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
    case Token_Equal:
        return OPERATION_ASSIGNMENT;
    case Token_Ou:
        return OPERATION_OR;
    case Token_E:
        return OPERATION_AND;
    case Token_Equal_Equal:
    case Token_Not_Equal:
        return OPERATION_EQUALITY;
    case Token_Greater:
    case Token_Less:
    case Token_Greater_Equal:
    case Token_Less_Equal:
        return OPERATION_COMPARISON;
    case Token_Plus:
    case Token_Minus:
        return OPERATION_ADDITION_AND_SUBTRACTION;
    case Token_Asterisk:
    case Token_Slash:
        return OPERATION_MULTIPLICATION_AND_DIVISION;
    case Token_Caret:
        return OPERATION_EXPONENTIATION;
    case Token_Ka:
        return OPERATION_NOT;
    case Token_Dot:
    case Token_Left_Parenthesis:
        return OPERATION_GROUPING_CALL_AND_GET;
    }
}

static Expression* parser_expression(Parser* parser, OrderOfOperation operator_precedence_previous)
{
    parser_advance(parser);

    bool can_assign = (operator_precedence_previous <= OPERATION_ASSIGNMENT);
    Expression* left_operand = parser_unary_and_literals(parser, can_assign);
    Expression* expression = left_operand;

    TokenKind operator_kind_current = parser->token_current.kind;
    OrderOfOperation operator_precedence_current = parser_operator_precedence(operator_kind_current);

    while (operator_precedence_current >= operator_precedence_previous) {
        // TODO(?): if parser->token_current.kind == Token_Equal : break;
        //          fix: 1 = 4; or var a = 1 * 3 + x = 4 - 6;
        parser_advance(parser);
        expression = parser_binary(parser, expression);

        operator_precedence_current = parser_operator_precedence(parser->token_current.kind);
    }

    if (can_assign && parser_match_then_advance(parser, Token_Equal)) {
        parser_error(parser, &parser->token_current, "Invalid assignment target.");
    }

    return expression;
}

static Expression* parser_unary_and_literals(Parser* parser, bool can_assign)
{
    // Literals
    //
    if (parser->token_previous.kind == Token_Number)
    {
        double value = strtod(parser->token_previous.start, NULL);
        Value number = value_make_number(value);
        Expression e_number = expression_make_number(value);

        bytecode_emit_constant(number, parser->token_previous.line_number);

        return expression_allocate(e_number);
    }

    if (parser->token_previous.kind == Token_Verdadi)
    {
        Expression e_true = expression_make_boolean(true);

        bytecode_emit_instruction_1byte(OpCode_True, parser->token_previous.line_number);
        return expression_allocate(e_true);
    }

    if (parser->token_previous.kind == Token_Falsu)
    {
        Expression e_false = expression_make_boolean(false);

        bytecode_emit_instruction_1byte(OpCode_False, parser->token_previous.line_number);
        return expression_allocate(e_false);
    }

    if (parser->token_previous.kind == Token_Nulo)
    {
        Expression nil = expression_make_nil();

        bytecode_emit_instruction_1byte(OpCode_Nil, parser->token_previous.line_number);
        return expression_allocate(nil);
    }

    if (parser->token_previous.kind == Token_String)
    {
        // Check if the source_string already exists in the global string
        // database in the virtual machine. If it does, reuse it, if not
        // allocate a new one and store it in the global string database for
        // future reference.
        //
        ObjectString* string = ObjectString_AllocateIfNotInterned(parser->token_previous.start + 1, parser->token_previous.length - 2);

        Value v_string = value_make_object(string);
        Expression e_string = expression_make_string(string);

        bytecode_emit_constant(v_string, parser->token_previous.line_number);
        return expression_allocate(e_string);
    }

    if (parser->token_previous.kind == Token_String_Interpolation)
    {
        for (;;)
        {
            if (parser->token_previous.kind != Token_String_Interpolation)
                break;

            ObjectString* string = ObjectString_AllocateIfNotInterned(parser->token_previous.start + 1, parser->token_previous.length - 2);
            Value v_string = value_make_object(string);
            Expression e_string = expression_make_string(string);

            // Tracks on many values have it pushed to the stack on runtime.
            //
            parser->interpolation_count_value_pushed += 1;
            bytecode_emit_constant(v_string, parser->token_previous.line_number);

            parser->interpolation_count_nesting += 1;
            if (parser->token_current.kind != Token_String_Interpolation)
                parser->interpolation_count_value_pushed += 1;
            parser_expression(parser, OPERATION_ASSIGNMENT);
            parser->interpolation_count_nesting -= 1;

            parser_advance(parser);
        }

        if (*parser->token_previous.start != '}')
            parser_error(parser, &parser->token_previous, "Missing closing '}' in interpolation template.");

        parser->interpolation_count_value_pushed += 1;
        parser_unary_and_literals(parser, can_assign);

        if (parser->interpolation_count_nesting == 0)
        {
            bytecode_emit_instruction_2bytes(
                OpCode_Interpolation,
                (uint8_t)parser->interpolation_count_value_pushed,
                parser->token_previous.line_number
            );
        }

        // TODO: implement string interpolation Ast-Node and return it
        //
        return NULL;
    }

    // Unary
    //
    if (parser->token_previous.kind == Token_Minus)
    {
        Expression* expression = parser_expression(parser, OPERATION_NEGATE);
        Expression negation = expression_make_negation(expression);

        bytecode_emit_instruction_1byte(OpCode_Negation, parser->token_previous.line_number);
        return expression_allocate(negation);
    }

    if (parser->token_previous.kind == Token_Ka)
    {
        Expression* expression = parser_expression(parser, OPERATION_NOT);
        Expression not = expression_make_not(expression);

        bytecode_emit_instruction_1byte(OpCode_Not, parser->token_previous.line_number);
        return expression_allocate(not);
    }

    if (parser->token_previous.kind == Token_Left_Parenthesis)
    {
        Expression* expression = parser_expression(parser, OPERATION_ASSIGNMENT);
        Expression grouping = expression_make_grouping(expression);

        parser_consume(parser, Token_Right_Parenthesis, "Expected ')' after expression.");
        return expression_allocate(grouping);
    }

    // Identifier
    //
    if (parser->token_previous.kind == Token_Identifier) {
        // TODO: Support for more than 256 value in a Scope

        uint8_t opcode_assign = OpCode_Invalid;
        uint8_t opcode_read = OpCode_Invalid;
        Local* local_found = NULL;
        int operand = StackLocal_get_local_index_by_token(&parser->scope_current->locals, &parser->token_previous, &local_found);

        if (operand != -1) {
            opcode_assign = OpCode_Assign_Local;
            opcode_read = OpCode_Read_Local;
        } else {
            // Default
            operand = parser_store_identifier_into_bytecode(parser, parser->token_previous.start, parser->token_previous.length);
            opcode_assign = OpCode_Assign_Global;
            opcode_read = OpCode_Read_Global;
        }

        if (local_found && local_found->scope_depth == -1)
            parser_error(parser, &parser->token_previous, "Can't read local variable in its own initializer.");

        // Assignment
        //
        if (can_assign && parser_match_then_advance(parser, Token_Equal)) {
            parser_expression(parser, OPERATION_ASSIGNMENT);
            bytecode_emit_instruction_2bytes(opcode_assign, operand, parser->token_previous.line_number);
        } else {
            bytecode_emit_instruction_2bytes(opcode_read, operand, parser->token_previous.line_number);
        }

        return NULL;
    }

    // TODO: log token name
    assert(false && "Unhandled token...");

    return NULL;
}

static Expression* parser_binary(Parser* parser, Expression* left_operand) {
    TokenKind operator_kind_previous = parser->token_previous.kind;
    // TODO(?): if parser->token_previous.kind == Token_Equal : return;

    if (operator_kind_previous == Token_E) {
        int operand_index = bytecode_emit_instruction_jump(OpCode_Jump_If_False, parser->token_previous.line_number);
        bytecode_emit_instruction_1byte(OpCode_Pop, parser->token_previous.line_number);

        parser_expression(parser, OPERATION_AND);

        Bytecode_PatchInstructionJump(operand_index);

        return NULL;
    }

    OrderOfOperation operator_precedence_previous = parser_operator_precedence(operator_kind_previous);
    Expression* right_operand = parser_expression(parser, operator_precedence_previous);

    if (operator_kind_previous == Token_Plus)
    {
        bytecode_emit_instruction_1byte(OpCode_Addition, parser->token_previous.line_number);

        Expression addition = expression_make_addition(left_operand, right_operand);
        return expression_allocate(addition);
    }

    if (operator_kind_previous == Token_Minus)
    {
        bytecode_emit_instruction_1byte(OpCode_Subtraction, parser->token_previous.line_number);

        Expression subtraction = expression_make_subtraction(left_operand, right_operand);
        return expression_allocate(subtraction);
    }

    if (operator_kind_previous == Token_Asterisk)
    {
        bytecode_emit_instruction_1byte(OpCode_Multiplication, parser->token_previous.line_number);

        Expression multiplication = expression_make_multiplication(left_operand, right_operand);
        return expression_allocate(multiplication);
    }

    if (operator_kind_previous == Token_Slash)
    {
        bytecode_emit_instruction_1byte(OpCode_Division, parser->token_previous.line_number);

        Expression division = expression_make_division(left_operand, right_operand);
        return expression_allocate(division);
    }

    if (operator_kind_previous == Token_Caret)
    {
        bytecode_emit_instruction_1byte(OpCode_Exponentiation, parser->token_previous.line_number);

        Expression exponentiation = expression_make_division(left_operand, right_operand);
        return expression_allocate(exponentiation);
    }

    if (operator_kind_previous == Token_Equal_Equal)
    {
        bytecode_emit_instruction_1byte(OpCode_Equal_To, parser->token_previous.line_number);

        Expression equal_to = expression_make_equal_to(left_operand, right_operand);
        return expression_allocate(equal_to);
    }

    if (operator_kind_previous == Token_Not_Equal)
    {
        // a != b has the same semantics as !(a == b)
        //
        bytecode_emit_instruction_1byte(OpCode_Equal_To, parser->token_previous.line_number);
        bytecode_emit_instruction_1byte(OpCode_Not, parser->token_previous.line_number);

        Expression* equal_to = expression_allocate(expression_make_equal_to(left_operand, right_operand));
        return expression_allocate(expression_make_not(equal_to));
    }

    if (operator_kind_previous == Token_Greater)
    {
        bytecode_emit_instruction_1byte(OpCode_Greater_Than, parser->token_previous.line_number);

        Expression greater_than = expression_make_greater_than(left_operand, right_operand);
        return expression_allocate(greater_than);
    }

    if (operator_kind_previous == Token_Greater_Equal)
    {
        // a >= b has the same semantics as !(a < b)
        //
        bytecode_emit_instruction_1byte(OpCode_Less_Than, parser->token_previous.line_number);
        bytecode_emit_instruction_1byte(OpCode_Not, parser->token_previous.line_number);

        Expression* greater_than = expression_allocate(expression_make_greater_than(left_operand, right_operand));
        Expression greater_than_or_equal_to = expression_make_not(greater_than);
        return expression_allocate(greater_than_or_equal_to);
    }

    if (operator_kind_previous == Token_Less)
    {
        bytecode_emit_instruction_1byte(OpCode_Less_Than, parser->token_previous.line_number);

        Expression less_than = expression_make_less_than(left_operand, right_operand);
        return expression_allocate(less_than);
    }

    if (operator_kind_previous == Token_Less_Equal)
    {
        // a <= b has the same semantics as !(a > b)
        //
        bytecode_emit_instruction_1byte(OpCode_Greater_Than, parser->token_previous.line_number);
        bytecode_emit_instruction_1byte(OpCode_Not, parser->token_previous.line_number);

        Expression* less_than = expression_allocate(expression_make_less_than(left_operand, right_operand));
        Expression less_than_or_equal_to = expression_make_not(less_than);
        return expression_allocate(less_than_or_equal_to);
    }

    // TODO: log token name
    assert(false && "Unhandled token...");

    return NULL;
}

static bool parser_check_locals_duplicates(Parser* parser, Token* identifier) {
    Scope* current_scope = parser->scope_current;
    for (int i = current_scope->locals.count - 1; i >= 0; i--) {
        Local* local = &current_scope->locals.items[i];
        if (local->scope_depth < current_scope->depth && local->scope_depth != -1) {
            break;
        }

        if (token_is_identifier_equal(identifier, &local->token))
            return true;

        // if (token_is_identifier_equal(identifier, &local->token)) {
        //     parser_error(parser, identifier, "Already a variable with this name in this scope.");
        // }
    }

    return false;
}

static void parser_end(Parser* parser) {
    bytecode_emit_instruction_1byte(OpCode_Return, parser->token_current.line_number);
}

static void parser_begin_scope(Parser* parser) {
    parser->scope_current->depth += 1;
}

static void parser_end_scope(Parser* parser) {
    parser->scope_current->depth -= 1;

    for (;;) {
        StackLocal locals = parser->scope_current->locals;

        if (locals.count <= 0) break;
        if (locals.items[locals.count - 1].scope_depth <= parser->scope_current->depth) break;

        bytecode_emit_instruction_1byte(OpCode_Pop, parser->token_previous.line_number);
        StackLocal_pop(&parser->scope_current->locals);
    }
}
// todo: parser destroy implementation