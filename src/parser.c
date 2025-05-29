#include "kriolu.h"

// TODO: [] parser destroy implementation

static void parser_advance(Parser* parser);
static void parser_consume(Parser* parser, TokenKind kind, const char* error_message);
static bool parser_match_then_advance(Parser* parser, TokenKind kind);
static void parser_synchronize(Parser* parser);
static void parser_error(Parser* parser, Token* token, const char* message);
static Statement* parser_parse_statement(Parser* parser, BlockType block_type);
static Statement* parser_parse_statement_expression(Parser* parser);
static Statement* parser_parse_instruction_print(Parser* parser);
static Statement* parser_parse_instruction_block(Parser* parser, BlockType is_pure_block);
static Statement* parser_parse_instruction_if(Parser* parser);
static Statement* parser_instruction_while(Parser* parser);
static Statement* parser_instruction_for(Parser* parser);
static Statement* parser_instruction_break(Parser* parser);
static Statement* parser_instruction_continue(Parser* parser);
static Statement* parser_parse_variable_declaration(Parser* parser);
static Expression* parser_parse_expression(Parser* parser, OperatorPrecedence operator_precedence_previous);
static Expression* parser_parse_unary_literals_and_identifier(Parser* parser, bool can_assign);
static Expression* parser_parse_operators_logical(Parser* parser, Expression* left_operand);
static Expression* parser_parse_operators_arithmetic(Parser* parser, Expression* left_operand);
static Expression* parser_parse_operators_relational(Parser* parser, Expression* left_operand);
static bool parser_is_operator_arithmetic(Parser* parser);
static bool parser_is_operator_logical(Parser* parser);
static bool parser_is_operator_relational(Parser* parser);
static bool parser_check_locals_duplicates(Parser* parser, Token* identifier);
static void parser_end_parsing(Parser* parser);
static void parser_begin_scope(Parser* parser);
static void parser_end_scope(Parser* parser);

// TODO: Delete 
static Expression* parser_parse_binary(Parser* parser, Expression* left_operand);
static uint8_t parser_store_identifier_into_bytecode(Parser* parser, const char* start, int length);


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
    StackBreak_init(&parser->breakpoints);
    StackBlock_init(&parser->blocks);
    parser->continue_jump_to = -1;
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
        Statement* statement = parser_parse_statement(parser, 1);
        if (statement == NULL) continue;

        array_statement_insert(statements, *statement);
        // TODO: free statement here
    }

    parser_end_parsing(parser);

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
    if (parser->panic_mode) return;

    FILE* error_output = stdout;

    parser->panic_mode = true;
    parser->had_error = true;

    fprintf(error_output, "[line %d] Error", token->line_number);
    if (token->kind == Token_Eof) {
        fprintf(error_output, " at end");
    } else if (token->kind == Token_Error) {
        fprintf(error_output, " at %.*s\n", token->length, token->start);
    } else {
        fprintf(error_output, " at '%.*s'", token->length, token->start);
        fprintf(error_output, " : '%s'\n", message);
    }
}

static Statement* parser_parse_statement(Parser* parser, BlockType block_type)
{
    // DECLARATIONS: introduces new identifiers
    //   klassi, mimoria, funson
    // INSTRUCTIONS: tells the computer to perform an action
    //   imprimi, di, si, divolvi, timenti, block
    // EXPRESSIONS: a calculation that produces a result
    //   +, -, /, *, call '(', assignment '=', objectGet '.'
    //   variableGet, literals

    Statement* statement = { 0 };

    if (parser_match_then_advance(parser, Token_Mimoria))
        statement = parser_parse_variable_declaration(parser);
    else if (parser_match_then_advance(parser, Token_Imprimi))
        statement = parser_parse_instruction_print(parser);
    else if (parser_match_then_advance(parser, Token_Si))
        statement = parser_parse_instruction_if(parser);
    else if (parser_match_then_advance(parser, Token_Left_Brace)) {
        parser_begin_scope(parser);
        statement = parser_parse_instruction_block(parser, block_type);
        parser_end_scope(parser);
    } else if (parser_match_then_advance(parser, Token_Timenti))
        statement = parser_instruction_while(parser);
    else if (parser_match_then_advance(parser, Token_Di))
        statement = parser_instruction_for(parser);
    else if (parser_match_then_advance(parser, Token_Pa))
        statement = parser_instruction_for(parser);
    else if (parser_match_then_advance(parser, Token_Sai))
        statement = parser_instruction_break(parser);
    else if (parser_match_then_advance(parser, Token_Salta))
        statement = parser_instruction_continue(parser);
    else statement = parser_parse_statement_expression(parser);

    if (parser->panic_mode)
        parser_synchronize(parser);

    return statement;
}

static Statement* parser_parse_statement_expression(Parser* parser)
{
    Expression* expression = parser_parse_expression(parser, OperatorPrecedence_Assignment);
    parser_consume(parser, Token_Semicolon, "Expect ';' after expression.");
    bytecode_emit_instruction_1byte(OpCode_Pop, parser->token_previous.line_number);

    Statement statement = (Statement){
        .kind = StatementKind_Expression,
        .expression = expression
    };

    return statement_allocate(statement);
}

static Statement* parser_parse_instruction_print(Parser* parser) {

    Expression* expression = parser_parse_expression(parser, OperatorPrecedence_Assignment);
    parser_consume(parser, Token_Semicolon, "Expect ';' after expression.");
    bytecode_emit_instruction_1byte(OpCode_Print, parser->token_previous.line_number);

    Statement statement = { 0 };
    statement.kind = StatementKind_Print;
    statement.print = expression;

    return statement_allocate(statement);
}

static void parser_begin_block(Parser* parser, BlockType block_type) {
    StackBlock_push(&parser->blocks, block_type);
}

static void parser_end_block(Parser* parser, BlockType block_type) {
    StackBlock* blocks = &parser->blocks;
    StackBreakpoint* breakpoints = &parser->breakpoints;
    int top_block_index = StackBlock_get_top_item_index(&parser->blocks);

    for (;;) {
        Breakpoint breakpoint = StackBreak_peek(breakpoints, 0);

        if (StackBreak_is_empty(&parser->breakpoints)) break;
        if (StackBlock_is_empty(&parser->blocks)) break;
        if (breakpoint.block_depth < top_block_index) break;

        Bytecode_PatchInstructionJump(breakpoint.operand_index);
        StackBreak_pop(breakpoints);
    }

    StackBlock_pop(blocks);
}
static Statement* parser_parse_instruction_block(Parser* parser, BlockType block_type) {
    if (block_type == BlockType_Clean) parser_begin_block(parser, block_type);

    ArrayStatement* bloco = array_statement_allocate();

    for (;;) {
        TokenKind kind = parser->token_current.kind;
        if (kind == Token_Right_Brace) break;
        if (kind == Token_Eof)         break;

        Statement* statement = parser_parse_statement(parser, block_type);
        if (statement == NULL) continue;

        array_statement_insert(bloco, *statement);
        // TODO: free statement
    }

    parser_consume(parser, Token_Right_Brace, "Expect '}' after block.");

    if (block_type == BlockType_Clean) parser_end_block(parser, block_type);

    Statement statement = { 0 };
    statement.kind = StatementKind_Block;
    statement.bloco = bloco;
    return statement_allocate(statement);
}

// if (condition) {then-block} sinou {else-block} 
//
static Statement* parser_parse_instruction_if(Parser* parser) {
    Statement statement = { 0 };
    statement.kind = StatementKind_Si;

    parser_consume(parser, Token_Left_Parenthesis, "Expect '(' after 'if'.");
    statement._if.condition = parser_parse_expression(parser, OperatorPrecedence_Assignment); // Will leave a Boolean value on top of the Stack
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
    statement._if.then_block = parser_parse_statement(parser, BlockType_If);

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

    if (parser_match_then_advance(parser, Token_Sinou))
        statement._if.else_block = parser_parse_statement(parser, BlockType_If);

    bool error = Bytecode_PatchInstructionJump(jump_operand_index);
    if (error) parser_error(parser, &parser->token_previous, "Too much code to jump over.");

    return statement_allocate(statement);
}

static Statement* parser_instruction_while(Parser* parser) {
    int loop_start = g_bytecode.instructions.count; // TODO: use a macro to access count variable

    parser_begin_block(parser, BlockType_Loop);

    parser_consume(parser, Token_Left_Parenthesis, "Expect '(' after 'timenti'.");
    Expression* condition = parser_parse_expression(parser, OperatorPrecedence_Assignment);
    parser_consume(parser, Token_Right_Parenthesis, "Expect ')' after condition.");

    int jump_if_false_operand_index = bytecode_emit_instruction_jump(OpCode_Jump_If_False, parser->token_previous.line_number);
    bytecode_emit_instruction_1byte(OpCode_Pop, parser->token_previous.line_number);

    Statement* body = parser_parse_statement(parser, false);

    Bytecode_EmitLoop(loop_start, parser->token_previous.line_number);
    Bytecode_PatchInstructionJump(jump_if_false_operand_index);
    bytecode_emit_instruction_1byte(OpCode_Pop, parser->token_previous.line_number);

    parser_end_block(parser, BlockType_Loop);

    Statement statement = { 0 };
    statement.kind = StatementKind_Timenti;
    statement.timenti.condition = condition;
    statement.timenti.body = body;
    return statement_allocate(statement);
}

static Statement* parser_instruction_for(Parser* parser) {
    parser_begin_scope(parser);
    parser_begin_block(parser, BlockType_Loop);

    parser_consume(parser, Token_Left_Parenthesis, "Expected '(' after 'pa'.");

    Statement* initializer = NULL;
    if (parser_match_then_advance(parser, Token_Semicolon)) {
        // Do nothing. There is no initializer.
    } else if (parser_match_then_advance(parser, Token_Mimoria)) {
        initializer = parser_parse_variable_declaration(parser);
    } else {
        initializer = parser_parse_statement_expression(parser);
    }

    int condition_start_index = g_bytecode.instructions.count;
    int exit_jump_operand_index = -1;
    Expression* condition = NULL;
    if (!parser_match_then_advance(parser, Token_Semicolon)) {
        condition = parser_parse_expression(parser, OperatorPrecedence_Assignment);
        parser_consume(parser, Token_Semicolon, "Epected ';'.");

        exit_jump_operand_index = bytecode_emit_instruction_jump(OpCode_Jump_If_False, parser->token_previous.line_number);
        bytecode_emit_instruction_1byte(OpCode_Pop, parser->token_previous.line_number);
    }

    Expression* increment = NULL;
    int increment_start_index = -1;
    int continue_jump_to_old = parser->continue_jump_to;
    if (!parser_match_then_advance(parser, Token_Right_Parenthesis)) {
        int jump_to_body = bytecode_emit_instruction_jump(OpCode_Jump, parser->token_previous.line_number);
        increment_start_index = g_bytecode.instructions.count;
        parser->continue_jump_to = increment_start_index;
        increment = parser_parse_expression(parser, OperatorPrecedence_Assignment);
        bytecode_emit_instruction_1byte(OpCode_Pop, parser->token_previous.line_number);
        parser_consume(parser, Token_Right_Parenthesis, "Epected ')' after increment expression clause.");

        Bytecode_EmitLoop(condition_start_index, parser->token_previous.line_number);
        Bytecode_PatchInstructionJump(jump_to_body);
    }

    Statement* body = parser_parse_statement(parser, BlockType_Loop); // false

    Bytecode_EmitLoop(increment_start_index, parser->token_previous.line_number);
    if (exit_jump_operand_index != -1) {
        Bytecode_PatchInstructionJump(exit_jump_operand_index);
        bytecode_emit_instruction_1byte(OpCode_Pop, parser->token_previous.line_number);
    }

    parser->continue_jump_to = continue_jump_to_old;
    parser_end_block(parser, BlockType_Loop);
    parser_end_scope(parser);

    Statement statement = { 0 };
    statement.kind = StatementKind_Pa;
    statement.pa.initializer = initializer;
    statement.pa.condition = condition;
    statement.pa.increment = increment;
    statement.pa.body = body;
    return statement_allocate(statement);
}

static Statement* parser_instruction_break(Parser* parser) {
    if (StackBlock_is_empty(&parser->blocks)) {
        parser_error(parser, &parser->token_previous, "Cannot exit top-level scope.");
        return NULL;
    }

    if (parser->breakpoints.top == BREAK_POINT_MAX) {
        parser_error(parser, &parser->token_previous, "Check your spaghetti code dude!");
        return NULL;
    }

    Breakpoint breakpoint = { 0 };
    breakpoint.operand_index = bytecode_emit_instruction_jump(OpCode_Jump, parser->token_previous.line_number);
    breakpoint.block_depth = StackBlock_get_top_item_index(&parser->blocks);
    StackBreak_push(&parser->breakpoints, breakpoint);

    parser_consume(parser, Token_Semicolon, "Expected ';' after token 'sai'.");

    Statement statement = { 0 };
    statement.kind = StatementKind_Sai;
    return statement_allocate(statement);
}

static Statement* parser_instruction_continue(Parser* parser) {
    if (parser->continue_jump_to == -1) {
        parser_error(parser, &parser->token_previous, "'salta' must be inside a loop");
        return NULL;
    }

    Bytecode_EmitLoop(parser->continue_jump_to, parser->token_previous.line_number);

    parser_consume(parser, Token_Semicolon, "Expected ';' after token 'salta'.");

    Statement statement = { 0 };
    statement.kind = StatementKind_Salta;
    return statement_allocate(statement);
}
static Statement* parser_parse_variable_declaration(Parser* parser) {
    Statement statement = { 0 };
    uint8_t global_index = 0;

    statement.kind = StatementKind_Variable_Declaration;

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
            return statement_allocate(statement);
        }

        StackLocal_push(
            &parser->scope_current->locals,
            parser->token_previous,
            -1 // Mark Local as Uninitialized 
        );

        statement.variable_declaration.identifier = ObjectString_AllocateIfNotInterned(parser->token_previous.start, parser->token_previous.length);

        // Check for assignment
        // 
        if (parser_match_then_advance(parser, Token_Equal)) {
            statement.variable_declaration.rhs = parser_parse_expression(parser, OperatorPrecedence_Assignment);
        } else {
            bytecode_emit_instruction_1byte(OpCode_Nil, parser->token_previous.line_number);
            statement.variable_declaration.rhs = expression_allocate(expression_make_nil());
        }

        parser_consume(parser, Token_Semicolon, "Expected ';' after expression.");

        // Mark local as Initialized
        //
        Local* local = &parser->scope_current->locals.items[parser->scope_current->locals.count - 1];
        local->scope_depth = parser->scope_current->depth;

        return statement_allocate(statement);
    }

        // Global Scope Only
        //
    statement.variable_declaration.identifier = ObjectString_AllocateIfNotInterned(parser->token_previous.start, parser->token_previous.length);
    Value value_string = value_make_object_string(statement.variable_declaration.identifier);

    int value_index = bytecode_emit_value(value_string);
    if (value_index > UINT8_MAX) {
        parser_error(parser, &parser->token_current, "Too many constants.");
        global_index = 0;
    } else {
        global_index = value_index;
    }

    // Check for assignment
    // 
    if (parser_match_then_advance(parser, Token_Equal)) {
        statement.variable_declaration.rhs = parser_parse_expression(parser, OperatorPrecedence_Assignment);
    } else {
        bytecode_emit_instruction_1byte(OpCode_Nil, parser->token_previous.line_number);
        statement.variable_declaration.rhs = expression_allocate(expression_make_nil());
    }

    parser_consume(parser, Token_Semicolon, "Expected ';' after expression.");

    // Define Global Varible
    //
    bytecode_emit_instruction_2bytes(
        OpCode_Define_Global,
        global_index,
        parser->token_previous.line_number
    );

    return statement_allocate(statement);
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


static OperatorMetadata parser_get_operator_metadata(TokenKind kind)
{
    switch (kind)
    {
    default: {
        OperatorMetadata operator_metadata = { 0 };
        operator_metadata.is_left_associative = true;
        return operator_metadata;
    }
    case Token_Equal: {
        OperatorMetadata operator_metadata = { 0 };
        operator_metadata.precedence = OperatorPrecedence_Assignment;
        operator_metadata.is_left_associative = false;
        operator_metadata.is_right_associative = true;
        return operator_metadata;
    }
    case Token_Ou: {
        // NOTE: Since its a bytecode interpreter, we use right-associativity when parsing 
        //       and emitting instructions to make jumps-instructions efficient, beacuse 
        //       we now the end of the statement while preserving left-associativity when 
        //       interpreting the code. 
        //       Otherwise if we were to use a AST interpreter, we would use left-associativity 
        //       when parsing to ensure that the interpretation will occour from left-to-right;
        OperatorMetadata operator_metadata = { 0 };
        operator_metadata.precedence = OperatorPrecedence_Or;
        operator_metadata.is_left_associative = false;
        operator_metadata.is_right_associative = true;
        return operator_metadata;
    }
    case Token_E: {
        // NOTE: Since its a bytecode interpreter, we use right-associativity when parsing 
        //       and emitting instructions to make jumps-instructions efficient, beacuse 
        //       we now the end of the statement while preserving left-associativity when 
        //       interpreting the code. 
        //       Otherwise if we were to use a AST interpreter, we would use left-associativity 
        //       when parsing to ensure that the interpretation will occour from left-to-right;
        OperatorMetadata operator_metadata = { 0 };
        operator_metadata.precedence = OperatorPrecedence_And;
        operator_metadata.is_left_associative = false;
        operator_metadata.is_right_associative = true;
        return operator_metadata;
    }
    case Token_Equal_Equal:
    case Token_Not_Equal: {
        OperatorMetadata operator_metadata = { 0 };
        operator_metadata.precedence = OperatorPrecedence_Equality;
        operator_metadata.is_left_associative = true;
        operator_metadata.is_right_associative = false;
        return operator_metadata;
    }
    case Token_Greater:
    case Token_Less:
    case Token_Greater_Equal:
    case Token_Less_Equal: {
        OperatorMetadata operator_metadata = { 0 };
        operator_metadata.precedence = OperatorPrecedence_Comparison;
        operator_metadata.is_left_associative = true;
        operator_metadata.is_right_associative = false;
        return operator_metadata;
    }
    case Token_Plus:
    case Token_Minus: {
        OperatorMetadata operator_metadata = { 0 };
        operator_metadata.precedence = OperatorPrecedence_Addition_And_Subtraction;
        operator_metadata.is_left_associative = true;
        operator_metadata.is_right_associative = false;
        return operator_metadata;
    }
    case Token_Asterisk:
    case Token_Slash: {
        OperatorMetadata operator_metadata = { 0 };
        operator_metadata.precedence = OperatorPrecedence_Multiplication_And_Division;
        operator_metadata.is_left_associative = true;
        operator_metadata.is_right_associative = false;
        return operator_metadata;
    }
    case Token_Caret: {
        OperatorMetadata operator_metadata = { 0 };
        operator_metadata.precedence = OperatorPrecedence_Exponentiation;
        operator_metadata.is_left_associative = false;
        operator_metadata.is_right_associative = true;
        return operator_metadata;
    }
    case Token_Ka: {
        OperatorMetadata operator_metadata = { 0 };
        operator_metadata.precedence = OperatorPrecedence_Not;
        operator_metadata.is_left_associative = true;
        operator_metadata.is_right_associative = false;
        return operator_metadata;
    }
    case Token_Dot:
    case Token_Left_Parenthesis: {
        OperatorMetadata operator_metadata = { 0 };
        operator_metadata.precedence = OperatorPrecedence_Grouping_Call_And_Get;
        operator_metadata.is_left_associative = true;
        operator_metadata.is_right_associative = false;
        return operator_metadata;
    }
    }
}

static Expression* parser_parse_expression(Parser* parser, OperatorPrecedence operator_precedence_previous) {
    parser_advance(parser);

    bool can_assign = (operator_precedence_previous <= OperatorPrecedence_Assignment);
    Expression* expression = parser_parse_unary_literals_and_identifier(parser, can_assign);

    TokenKind operator_kind_current = parser->token_current.kind;
    OperatorMetadata operator_current = parser_get_operator_metadata(operator_kind_current);

    for (;;) {
        if (operator_current.is_left_associative && operator_current.precedence <= operator_precedence_previous) break;
        if (operator_current.is_right_associative && operator_current.precedence < operator_precedence_previous) break;
        if (parser->token_current.kind == Token_Equal) break;

        parser_advance(parser);

        if (parser_is_operator_arithmetic(parser)) {
            expression = parser_parse_operators_arithmetic(parser, expression);
        } else if (parser_is_operator_logical(parser)) {
            expression = parser_parse_operators_logical(parser, expression);
        } else if (parser_is_operator_relational(parser)) {
            expression = parser_parse_operators_relational(parser, expression);
        }

        operator_current = parser_get_operator_metadata(parser->token_current.kind);
    }

    if (can_assign && parser_match_then_advance(parser, Token_Equal)) {
        parser_error(parser, &parser->token_current, "Invalid assignment target.");
    }

    return expression;
}

static Expression* parser_parse_unary_literals_and_identifier(Parser* parser, bool can_assign)
{
    // Literals
    //
    if (parser->token_previous.kind == Token_Number) {
        double value = strtod(parser->token_previous.start, NULL);
        Value number = value_make_number(value);
        Expression e_number = expression_make_number(value);

        bytecode_emit_constant(number, parser->token_previous.line_number);

        return expression_allocate(e_number);
    }

    if (parser->token_previous.kind == Token_Verdadi) {
        Expression e_true = expression_make_boolean(true);

        bytecode_emit_instruction_1byte(OpCode_True, parser->token_previous.line_number);
        return expression_allocate(e_true);
    }

    if (parser->token_previous.kind == Token_Falsu) {
        Expression e_false = expression_make_boolean(false);

        bytecode_emit_instruction_1byte(OpCode_False, parser->token_previous.line_number);
        return expression_allocate(e_false);
    }

    if (parser->token_previous.kind == Token_Nulo) {
        Expression nil = expression_make_nil();

        bytecode_emit_instruction_1byte(OpCode_Nil, parser->token_previous.line_number);
        return expression_allocate(nil);
    }

    if (parser->token_previous.kind == Token_String) {
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

    if (parser->token_previous.kind == Token_String_Interpolation) {
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

            parser_parse_expression(parser, OperatorPrecedence_Assignment);
            parser->interpolation_count_nesting -= 1;

            parser_advance(parser);
        }

        if (*parser->token_previous.start != '}')
            parser_error(parser, &parser->token_previous, "Missing closing '}' in interpolation template.");

        parser->interpolation_count_value_pushed += 1;
        parser_parse_unary_literals_and_identifier(parser, can_assign);

        if (parser->interpolation_count_nesting == 0) {
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
    if (parser->token_previous.kind == Token_Minus) {
        Expression* expression = parser_parse_expression(parser, OperatorPrecedence_Negate);
        Expression negation = expression_make_negation(expression);

        bytecode_emit_instruction_1byte(OpCode_Negation, parser->token_previous.line_number);
        return expression_allocate(negation);
    }

    if (parser->token_previous.kind == Token_Ka) {
        Expression* expression = parser_parse_expression(parser, OperatorPrecedence_Not);
        Expression not = expression_make_not(expression);

        bytecode_emit_instruction_1byte(OpCode_Not, parser->token_previous.line_number);
        return expression_allocate(not);
    }

    if (parser->token_previous.kind == Token_Left_Parenthesis) {
        Expression* expression = parser_parse_expression(parser, OperatorPrecedence_Assignment);
        Expression grouping = expression_make_grouping(expression);

        parser_consume(parser, Token_Right_Parenthesis, "Expected ')' after expression.");
        return expression_allocate(grouping);
    }

    // Identifier
    //
    if (parser->token_previous.kind == Token_Identifier) {
        uint8_t opcode_assign = OpCode_Invalid;
        uint8_t opcode_read = OpCode_Invalid;
        Local* local_found = NULL;
        int operand = StackLocal_get_local_index_by_token(&parser->scope_current->locals, &parser->token_previous, &local_found);
        ObjectString* variable_name = ObjectString_AllocateIfNotInterned(parser->token_previous.start, parser->token_previous.length);

        if (operand != -1) {
            opcode_assign = OpCode_Assign_Local;
            opcode_read = OpCode_Read_Local;
        } else {
            // Default
            opcode_assign = OpCode_Assign_Global;
            opcode_read = OpCode_Read_Global;

            Value value_string = value_make_object_string(variable_name);

            int value_index = bytecode_emit_value(value_string);
            // TODO: Support for more than 256 value in a Scope
            if (value_index <= UINT8_MAX) {
                operand = value_index;
            } else {
                parser_error(parser, &parser->token_current, "Too many constants.");
                operand = 0;
            }

        }

        if (local_found && local_found->scope_depth == -1)
            parser_error(parser, &parser->token_previous, "Can't read local variable in its own initializer.");

        // Assignment / Variable Initialization 
        //
        if (can_assign && parser_match_then_advance(parser, Token_Equal)) {
            Expression* right_hand_side = parser_parse_expression(parser, OperatorPrecedence_Assignment);
            Expression expression = expression_make_assignment(variable_name, right_hand_side);
            bytecode_emit_instruction_2bytes(opcode_assign, operand, parser->token_previous.line_number);

            return expression_allocate(expression);
        }

        // Get variable value
        //
        Expression expression = expression_make_variable(variable_name);
        bytecode_emit_instruction_2bytes(opcode_read, operand, parser->token_previous.line_number);
        return expression_allocate(expression);
    }

    // TODO: log token name
    assert(false && "Unhandled token...");

    return NULL;
}

static bool parser_is_operator_arithmetic(Parser* parser) {
    TokenKind token_kind = parser->token_previous.kind;
    return (token_kind == Token_Plus ||
            token_kind == Token_Minus ||
            token_kind == Token_Asterisk ||
            token_kind == Token_Slash ||
            token_kind == Token_Caret);
}

static bool parser_is_operator_logical(Parser* parser) {
    TokenKind token_kind = parser->token_previous.kind;
    return (token_kind == Token_E || token_kind == Token_Ou || token_kind == Token_Ka);
}

static bool parser_is_operator_relational(Parser* parser) {
    TokenKind token_kind = parser->token_previous.kind;
    return (token_kind == Token_Equal_Equal ||
            token_kind == Token_Not_Equal ||
            token_kind == Token_Less ||
            token_kind == Token_Less_Equal ||
            token_kind == Token_Greater ||
            token_kind == Token_Greater_Equal);
}

static Expression* parser_parse_operators_logical(Parser* parser, Expression* left_operand) {
    TokenKind operator_kind_previous = parser->token_previous.kind;

    switch (parser->token_previous.kind)
    {
    default: {
        assert(false && "Error: Unhandled Logical Operator.");
        return NULL;
    } break;
    case Token_E: {
        int operand_index = bytecode_emit_instruction_jump(OpCode_Jump_If_False, parser->token_previous.line_number);
        bytecode_emit_instruction_1byte(OpCode_Pop, parser->token_previous.line_number);

        Expression* rigth_operand = parser_parse_expression(parser, OperatorPrecedence_And);

        Bytecode_PatchInstructionJump(operand_index);

        Expression expression_and = expression_make_and(left_operand, rigth_operand);
        return expression_allocate(expression_and);
    } break;
    case Token_Ou: {
        int jump_if_false_operand_index = bytecode_emit_instruction_jump(OpCode_Jump_If_False, parser->token_previous.line_number);
        int jump_operand_index = bytecode_emit_instruction_jump(OpCode_Jump, parser->token_previous.line_number);

        Bytecode_PatchInstructionJump(jump_if_false_operand_index);
        bytecode_emit_instruction_1byte(OpCode_Pop, parser->token_previous.line_number);

        Expression* rigth_operand = parser_parse_expression(parser, OperatorPrecedence_Or);
        Bytecode_PatchInstructionJump(jump_operand_index);

        Expression expression_or = expression_make_or(left_operand, rigth_operand);
        return expression_allocate(expression_or);
    } break;
    }

}

static Expression* parser_parse_operators_arithmetic(Parser* parser, Expression* left_operand) {
    TokenKind operator_kind_previous = parser->token_previous.kind;
    OperatorMetadata operator_previous = parser_get_operator_metadata(operator_kind_previous);
    Expression* right_operand = parser_parse_expression(parser, operator_previous.precedence);

    switch (operator_kind_previous)
    {
    case Token_Plus: {
        bytecode_emit_instruction_1byte(OpCode_Addition, parser->token_previous.line_number);
        Expression addition = expression_make_addition(left_operand, right_operand);
        return expression_allocate(addition);
    } break;
    case Token_Minus: {
        bytecode_emit_instruction_1byte(OpCode_Subtraction, parser->token_previous.line_number);
        Expression subtraction = expression_make_subtraction(left_operand, right_operand);
        return expression_allocate(subtraction);
    } break;
    case Token_Asterisk: {
        bytecode_emit_instruction_1byte(OpCode_Multiplication, parser->token_previous.line_number);
        Expression multiplication = expression_make_multiplication(left_operand, right_operand);
        return expression_allocate(multiplication);
    } break;
    case Token_Slash: {
        bytecode_emit_instruction_1byte(OpCode_Division, parser->token_previous.line_number);
        Expression division = expression_make_division(left_operand, right_operand);
        return expression_allocate(division);
    } break;
    case Token_Caret: {
        bytecode_emit_instruction_1byte(OpCode_Exponentiation, parser->token_previous.line_number);
        Expression exponentiation = expression_make_exponentiation(left_operand, right_operand);
        return expression_allocate(exponentiation);
    } break;
    }

    assert(false && "Error: Unhandled Arithmetic Operator.");
    return NULL;
}

static Expression* parser_parse_operators_relational(Parser* parser, Expression* left_operand) {
    TokenKind operator_kind_previous = parser->token_previous.kind;
    OperatorMetadata operator_previous = parser_get_operator_metadata(operator_kind_previous);
    Expression* right_operand = parser_parse_expression(parser, operator_previous.precedence);

    switch (operator_kind_previous)
    {
    case Token_Equal_Equal: {
        bytecode_emit_instruction_1byte(OpCode_Equal_To, parser->token_previous.line_number);
        Expression equal_to = expression_make_equal_to(left_operand, right_operand);
        return expression_allocate(equal_to);
    } break;
    case Token_Not_Equal: {
        // a != b has the same semantics as !(a == b)
        //
        bytecode_emit_instruction_1byte(OpCode_Equal_To, parser->token_previous.line_number);
        bytecode_emit_instruction_1byte(OpCode_Not, parser->token_previous.line_number);

        Expression* equal_to = expression_allocate(expression_make_equal_to(left_operand, right_operand));
        return expression_allocate(expression_make_not(equal_to));
    } break;
    case Token_Greater: {
        bytecode_emit_instruction_1byte(OpCode_Greater_Than, parser->token_previous.line_number);

        Expression greater_than = expression_make_greater_than(left_operand, right_operand);
        return expression_allocate(greater_than);
    } break;
    case Token_Greater_Equal: {
        // a >= b has the same semantics as !(a < b)
        //
        bytecode_emit_instruction_1byte(OpCode_Less_Than, parser->token_previous.line_number);
        bytecode_emit_instruction_1byte(OpCode_Not, parser->token_previous.line_number);

        Expression* greater_than = expression_allocate(expression_make_greater_than(left_operand, right_operand));
        Expression greater_than_or_equal_to = expression_make_not(greater_than);
        return expression_allocate(greater_than_or_equal_to);
    } break;
    case Token_Less: {
        bytecode_emit_instruction_1byte(OpCode_Less_Than, parser->token_previous.line_number);

        Expression less_than = expression_make_less_than(left_operand, right_operand);
        return expression_allocate(less_than);
    } break;
    case Token_Less_Equal: {
        // a <= b has the same semantics as !(a > b)
        //
        bytecode_emit_instruction_1byte(OpCode_Greater_Than, parser->token_previous.line_number);
        bytecode_emit_instruction_1byte(OpCode_Not, parser->token_previous.line_number);

        Expression less_than_or_equal_to = expression_make_less_than_or_equal_to(left_operand, right_operand);
        return expression_allocate(less_than_or_equal_to);
    } break;
    }

    assert(false && "Error: Unhandled Relational Operator.");
    return NULL;
}

static Expression* parser_parse_binary(Parser* parser, Expression* left_operand) {
    TokenKind operator_kind_previous = parser->token_previous.kind;

    // 
    // Logical Operations: 
    //     and, or, not 
    //

    if (operator_kind_previous == Token_E) {
        int operand_index = bytecode_emit_instruction_jump(OpCode_Jump_If_False, parser->token_previous.line_number);
        bytecode_emit_instruction_1byte(OpCode_Pop, parser->token_previous.line_number);

        Expression* expression = parser_parse_expression(parser, OperatorPrecedence_And);

        Bytecode_PatchInstructionJump(operand_index);

        return NULL;
    }

    // 
    // Arithmetic Operations: 
    //     plus, minus, multiplications, division, exponentiation, and logarithm
    //

    OperatorMetadata operator_previous = parser_get_operator_metadata(operator_kind_previous);
    Expression* right_operand = parser_parse_expression(parser, operator_previous.precedence);

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

    //
    // Comparison or Relational Operations
    // 

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

static void parser_end_parsing(Parser* parser) {
    bytecode_emit_instruction_1byte(OpCode_Return, parser->token_current.line_number);
}

static void parser_begin_scope(Parser* parser) {
    parser->scope_current->depth += 1;
}

static void parser_end_scope(Parser* parser) {
    parser->scope_current->depth -= 1;
    // TODO: refactor StackLocal by adding function like peek and get_top_item_index
    for (;;) {
        StackLocal locals = parser->scope_current->locals;

        if (locals.count <= 0) break;
        if (locals.items[locals.count - 1].scope_depth <= parser->scope_current->depth) break;

        bytecode_emit_instruction_1byte(OpCode_Pop, parser->token_previous.line_number);
        StackLocal_pop(&parser->scope_current->locals);
    }
}

// todo: parser destroy implementation