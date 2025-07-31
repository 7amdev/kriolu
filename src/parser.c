#include "kriolu.h"

// TODO: [] parser destroy implementation

static void parser_advance(Parser* parser);
static void parser_consume(Parser* parser, TokenKind kind, const char* error_message);
static bool parser_match_then_advance(Parser* parser, TokenKind kind);
static void parser_synchronize(Parser* parser);
static void parser_error(Parser* parser, Token* token, const char* message);
static Bytecode* parser_get_current_bytecode(Parser* parser);
static Statement* parser_parse_statement(Parser* parser, BlockType block_type);
static Statement* parser_parse_instruction_print(Parser* parser);
static Statement* parser_parse_instruction_block(Parser* parser, BlockType is_pure_block);
static Statement* parser_parse_instruction_if(Parser* parser);
static Statement* parser_instruction_while(Parser* parser);
static Statement* parser_instruction_for(Parser* parser);
static Statement* parser_instruction_break(Parser* parser);
static Statement* parser_instruction_continue(Parser* parser);
static Statement* parser_instruction_return(Parser* parser);
static Statement* parser_parse_declaration_function(Parser* parser);
static Statement* parser_parse_declaration_variable(Parser* parser);
static int parser_parse_identifier(Parser* parser, const char* error_message);
static void parser_initialize_local_identifier(Parser* parser);
static ObjectFunction* parser_parse_function_paramenters_and_body(Parser* parser, FunctionKind function_kind);
static Statement* parser_parse_statement_expression(Parser* parser); // TODO: rename to ???
static Expression* parser_parse_expression(Parser* parser, OperatorPrecedence operator_precedence_previous);
static Expression* parser_parse_unary_literals_and_identifier(Parser* parser, bool can_assign);
static Expression* parser_parse_operator_function_call(Parser* parser, Expression* left_operand);
static Expression* parser_parse_operators_logical(Parser* parser, Expression* left_operand);
static Expression* parser_parse_operators_arithmetic(Parser* parser, Expression* left_operand);
static Expression* parser_parse_operators_relational(Parser* parser, Expression* left_operand);
// TODO: rename to Compiler_find_local_by_token(Compiler* compiler, Token* name, Local** out);
static void parser_compile_return(Parser* parser);
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

void parser_init(Parser* parser, const char* source_code, Lexer* lexer, HashTable* string_database, Object** object_head) {
    parser->token_current = (Token){ 0 };  // token_error
    parser->token_previous = (Token){ 0 }; // token_error
    parser->panic_mode = false;
    parser->had_error = false;
    parser->lexer = lexer;
    if (lexer == NULL) {
        parser->lexer = lexer_create_static();
        assert(parser->lexer);
        lexer_init(parser->lexer, source_code);
    }
    parser->compiler = NULL;
    parser->interpolation_count_nesting = 0;
    parser->interpolation_count_value_pushed = 0;
    parser->continue_jump_to = -1;
    parser->string_database = string_database;
    parser->object_head = object_head;
    StackBreak_init(&parser->breakpoints);
    StackBlock_init(&parser->blocks);
}

ObjectFunction* parser_parse(Parser* parser, ArrayStatement** return_statements)
{
    Compiler compiler;
    ArrayStatement* statements = array_statement_allocate();

    parser_advance(parser);
    Compiler_init(&compiler, FunctionKind_Script, &parser->compiler, NULL, parser->object_head);

    for (;;) {
        if (parser->token_current.kind == Token_Eof) break;

        Statement* statement = parser_parse_statement(parser, BlockType_Clean);
        if (statement == NULL) continue;

        array_statement_insert(statements, *statement);
        // TODO: free statement here
    }

    ObjectFunction* compiled_script = Compiler_end(&compiler, &parser->compiler, parser->had_error, parser->token_current.line_number);

    if (return_statements != NULL) *return_statements = statements;
    return parser->had_error ? NULL : compiled_script;
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

static Bytecode* parser_get_current_bytecode(Parser* parser) {
    return &parser->compiler->function->bytecode;
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

    if (parser_match_then_advance(parser, Token_Funson))
        statement = parser_parse_declaration_function(parser);
    else if (parser_match_then_advance(parser, Token_Mimoria))
        statement = parser_parse_declaration_variable(parser);
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
    else if (parser_match_then_advance(parser, Token_Divolvi))
        statement = parser_instruction_return(parser);
    else statement = parser_parse_statement_expression(parser);

    if (parser->panic_mode)
        parser_synchronize(parser);

    return statement;
}

static Statement* parser_parse_statement_expression(Parser* parser)
{
    Expression* expression = parser_parse_expression(parser, OperatorPrecedence_Assignment);
    parser_consume(parser, Token_Semicolon, "Expect ';' after expression.");

    Compiler_CompileInstruction_1Byte(parser_get_current_bytecode(parser), OpCode_Pop, parser->token_previous.line_number);

    Statement statement = (Statement){
        .kind = StatementKind_Expression,
        .expression = expression
    };

    return statement_allocate(statement);
}

static Statement* parser_parse_instruction_print(Parser* parser) {

    Expression* expression = parser_parse_expression(parser, OperatorPrecedence_Assignment);
    parser_consume(parser, Token_Semicolon, "Expect ';' after expression.");
    Compiler_CompileInstruction_1Byte(parser_get_current_bytecode(parser), OpCode_Print, parser->token_previous.line_number);

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

        Compiler_PatchInstructionJump(parser_get_current_bytecode(parser), breakpoint.operand_index);
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
    int jump_if_false_operand_index = Compiler_CompileInstruction_Jump(parser_get_current_bytecode(parser), OpCode_Jump_If_False, parser->token_previous.line_number);

    //
    // Then-Block
    //

    // Removes the condition's value from the stack, since a statement leaves NO value in the
    // stack.
    //
    Compiler_CompileInstruction_1Byte(parser_get_current_bytecode(parser), OpCode_Pop, parser->token_previous.line_number);

    // Compile then-block statements 
    // 
    statement._if.then_block = parser_parse_statement(parser, BlockType_If);

    // After finishing compiling statements in the then-block, jump out off 
    // the if-statement skipping the else-block statements.
    //
    int jump_operand_index = Compiler_CompileInstruction_Jump(parser_get_current_bytecode(parser), OpCode_Jump, parser->token_previous.line_number);

    // Patches the instruction 'jump_if_false' operand with the starting else-block index.
    //
    bool patch_error = Compiler_PatchInstructionJump(parser_get_current_bytecode(parser), jump_if_false_operand_index);
    if (patch_error) parser_error(parser, &parser->token_previous, "Too much code to jump over.");

    //
    // Else-Block
    //

    // Removes the condition's value from the stack, since a statement leaves NO value in the
    // stack.
    //
    Compiler_CompileInstruction_1Byte(parser_get_current_bytecode(parser), OpCode_Pop, parser->token_previous.line_number);

    if (parser_match_then_advance(parser, Token_Sinou))
        statement._if.else_block = parser_parse_statement(parser, BlockType_If);

    bool error = Compiler_PatchInstructionJump(parser_get_current_bytecode(parser), jump_operand_index);
    if (error) parser_error(parser, &parser->token_previous, "Too much code to jump over.");

    return statement_allocate(statement);
}

static Statement* parser_instruction_while(Parser* parser) {
    // int loop_start = g_bytecode.instructions.count; 
    int loop_start = parser_get_current_bytecode(parser)->instructions.count;

    parser_begin_block(parser, BlockType_Loop);

    parser_consume(parser, Token_Left_Parenthesis, "Expect '(' after 'timenti'.");
    Expression* condition = parser_parse_expression(parser, OperatorPrecedence_Assignment);
    parser_consume(parser, Token_Right_Parenthesis, "Expect ')' after condition.");

    int jump_if_false_operand_index = Compiler_CompileInstruction_Jump(parser_get_current_bytecode(parser), OpCode_Jump_If_False, parser->token_previous.line_number);
    Compiler_CompileInstruction_1Byte(parser_get_current_bytecode(parser), OpCode_Pop, parser->token_previous.line_number);

    Statement* body = parser_parse_statement(parser, false);

    Compiler_CompileInstruction_Loop(parser_get_current_bytecode(parser), loop_start, parser->token_previous.line_number);
    Compiler_PatchInstructionJump(parser_get_current_bytecode(parser), jump_if_false_operand_index);
    Compiler_CompileInstruction_1Byte(parser_get_current_bytecode(parser), OpCode_Pop, parser->token_previous.line_number);

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

#ifdef DEBUG_TRACE_INSTRUCTION
    printf("------ FOR-LOOP Instruction ------\n");
#endif

#ifdef DEBUG_TRACE_INSTRUCTION
    printf("------ Initialization ------\n");
#endif
    Statement* initializer = NULL;
    if (parser_match_then_advance(parser, Token_Semicolon)) {
        // Do nothing. There is no initializer.
    } else if (parser_match_then_advance(parser, Token_Mimoria)) {
        initializer = parser_parse_declaration_variable(parser);
    } else {
        initializer = parser_parse_statement_expression(parser);
    }
#ifdef DEBUG_TRACE_INSTRUCTION
    printf("--------------------------\n");
#endif

    int condition_start_index = parser_get_current_bytecode(parser)->instructions.count;
    int exit_jump_operand_index = -1;
    Expression* condition = NULL;
#ifdef DEBUG_TRACE_INSTRUCTION
    printf("------ Conditional ------\n");
#endif
    if (!parser_match_then_advance(parser, Token_Semicolon)) {
        condition = parser_parse_expression(parser, OperatorPrecedence_Assignment);
        parser_consume(parser, Token_Semicolon, "Epected ';'.");

        exit_jump_operand_index = Compiler_CompileInstruction_Jump(
            parser_get_current_bytecode(parser),
            OpCode_Jump_If_False,
            parser->token_previous.line_number
        );
        Compiler_CompileInstruction_1Byte(
            parser_get_current_bytecode(parser),
            OpCode_Pop,
            parser->token_previous.line_number
        );
    }
#ifdef DEBUG_TRACE_INSTRUCTION
    printf("--------------------------\n");
#endif

    Expression* increment = NULL;
    int increment_start_index = -1;
    int continue_jump_to_old = parser->continue_jump_to;

#ifdef DEBUG_TRACE_INSTRUCTION
    printf("------ Update ------\n");
#endif
    if (!parser_match_then_advance(parser, Token_Right_Parenthesis)) {
        int jump_to_body = Compiler_CompileInstruction_Jump(
            parser_get_current_bytecode(parser),
            OpCode_Jump,
            parser->token_previous.line_number
        );

        increment_start_index = parser_get_current_bytecode(parser)->instructions.count;
        parser->continue_jump_to = increment_start_index;
        increment = parser_parse_expression(parser, OperatorPrecedence_Assignment);

        Compiler_CompileInstruction_1Byte(
            parser_get_current_bytecode(parser),
            OpCode_Pop,
            parser->token_previous.line_number
        );

        // parser_get_current_bytecode(parser)->instructions.count
        parser_consume(parser, Token_Right_Parenthesis, "Epected ')' after increment expression clause.");

        Compiler_CompileInstruction_Loop(
            parser_get_current_bytecode(parser),
            condition_start_index,
            parser->token_previous.line_number
        );
        Compiler_PatchInstructionJump(
            parser_get_current_bytecode(parser),
            jump_to_body
        );
    }
#ifdef DEBUG_TRACE_INSTRUCTION
    printf("--------------------------\n");
#endif

#ifdef DEBUG_TRACE_INSTRUCTION
    printf("------ Body ------\n");
#endif

    Statement* body = parser_parse_statement(parser, BlockType_Loop);

#ifdef DEBUG_TRACE_INSTRUCTION
    printf("--------------------------\n");
#endif

    Compiler_CompileInstruction_Loop(
        parser_get_current_bytecode(parser),
        increment_start_index,
        parser->token_previous.line_number
    );

    if (exit_jump_operand_index != -1) {
        Compiler_PatchInstructionJump(
            parser_get_current_bytecode(parser),
            exit_jump_operand_index
        );
        Compiler_CompileInstruction_1Byte(
            parser_get_current_bytecode(parser),
            OpCode_Pop,
            parser->token_previous.line_number
        );
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
    breakpoint.operand_index = Compiler_CompileInstruction_Jump(parser_get_current_bytecode(parser), OpCode_Jump, parser->token_previous.line_number);
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

    Compiler_CompileInstruction_Loop(
        parser_get_current_bytecode(parser),
        parser->continue_jump_to,
        parser->token_previous.line_number
    );

    parser_consume(parser, Token_Semicolon, "Expected ';' after token 'salta'.");

    Statement statement = { 0 };
    statement.kind = StatementKind_Salta;
    return statement_allocate(statement);
}

static void parser_compile_return(Parser* parser) {
    Compiler_CompileInstruction_1Byte(
        parser_get_current_bytecode(parser),
        OpCode_Nil,
        parser->token_current.line_number
    );

    Compiler_CompileInstruction_1Byte(
        parser_get_current_bytecode(parser),
        OpCode_Return,
        parser->token_current.line_number
    );
}

static Statement* parser_instruction_return(Parser* parser) {
    if (parser->compiler->function_kind == FunctionKind_Script)
        parser_error(parser, &parser->token_previous, "Can't return from top-level code.");

    if (parser->token_current.kind == Token_Semicolon) {
        parser_compile_return(parser);
    } else {
        Expression* expression = parser_parse_expression(parser, OperatorPrecedence_Assignment);
        parser_consume(parser, Token_Semicolon, "Expect ';' after expression.");
        Compiler_CompileInstruction_1Byte(
            parser_get_current_bytecode(parser),
            OpCode_Return,
            parser->token_current.line_number
        );
    }

    Statement statement = { 0 };
    statement.kind = StatementKind_Return;
    return statement_allocate(statement);
}

static int parser_parse_identifier(Parser* parser, const char* error_message) {
    int value_index = 0;

    parser_consume(parser, Token_Identifier, error_message);

    if (parser->compiler->depth == 0) { // Parse Global Function Name 
        // ObjectString* identifier_string = ObjectString_allocate_if_not_interned(parser->string_database,parser->token_previous.start, parser->token_previous.length);
        String token = { 0 };
        string_init(&token, parser->token_previous.start, parser->token_previous.length);
        ObjectString* identifier_string = ObjectString_is_interned(parser->string_database, token);
        if (identifier_string == NULL) {
            uint32_t token_hash = string_hash(token);
            identifier_string = ObjectString_allocate_and_intern(parser->string_database, token.characters, token.length, token_hash, parser->object_head);
        }
        Value value_string = value_make_object_string(identifier_string);
        value_index = Compiler_CompileValue(parser_get_current_bytecode(parser), value_string);
        if (value_index > UINT8_MAX)
            parser_error(parser, &parser->token_current, "Too many constants.");
    } else {  // Parse Local Function Name 
        if (StackLocal_is_full(&parser->compiler->locals)) {
            parser_error(parser, &parser->token_previous, "Too many local variables in a scope.");
        }

        if (parser_check_locals_duplicates(parser, &parser->token_previous)) {
            parser_error(parser, &parser->token_previous, "Already a variable with this name in this scope.");
        }

        StackLocal_push(
            &parser->compiler->locals,
            parser->token_previous,
            -1, // Mark as Uninitialized
            false
        );
    }

    return value_index;
}

static void parser_initialize_local_identifier(Parser* parser) {
    if (parser->compiler->depth == 0) return;

    Local* local = StackLocal_peek(&parser->compiler->locals, 0);
    local->scope_depth = parser->compiler->depth;
}

static Statement* parser_parse_declaration_function(Parser* parser) {
    Statement statement = { 0 };
    statement.kind = StatementKind_Funson;

    int function_name_index = parser_parse_identifier(parser, "Expect function name identifier.");
    parser_initialize_local_identifier(parser);

    ObjectFunction* parsed_function = parser_parse_function_paramenters_and_body(parser, FunctionKind_Function);

    // Define Function Identifier
    //
    if (parser->compiler->depth == 0) {
        Compiler_CompileInstruction_2Bytes(
            parser_get_current_bytecode(parser),
            OpCode_Define_Global,
            function_name_index,
            parser->token_previous.line_number
        );
    }

    return statement_allocate(statement);
}

static Statement* parser_parse_method_declaration(Parser* parser) {
    // int method_name_index = parser_parse_identifier(parser, "Expect Method name identifier.");

    // FunctionKind function_kind = FunctionKind_Method;
    // if (parser.previous.length == 4 && memcmp(parser.previous.start, "init", 4) == 0)
    //     function_kind = FunctionKind_Initializer;
    // ObjectFunction* function = parser_parse_function_paramenters_and_body(parser, function_kind);

    // Value value = value_make_object(function);
    // Compiler_CompileInstruction_Constant(
    //     parser_get_current_bytecode(parser),
    //     value,
    //     parser->token_previous.line_number
    // );
    // 
    // Loop for every upvalue_count
    //     Compiler_CompileInstruction_2Bytes(is_local, index);

    // Define Method 
    //
    // Compiler_CompileInstruction_2Bytes(
    //     parser_get_current_bytecode(parser),
    //     OpCode_Define_Method,
    //     method_name_index,
    //     parser->token_previous.line_number
    // );
    return NULL;
}

static ObjectFunction* parser_parse_function_paramenters_and_body(Parser* parser, FunctionKind function_kind) {
    Compiler compiler;
    ObjectString* function_name = ObjectString_allocate_if_not_interned(parser->string_database, parser->token_previous.start, parser->token_previous.length, parser->object_head);

    // REVIEW: parser->object_head -> &parser->object_head
    //
    Compiler_init(&compiler, function_kind, &parser->compiler, function_name, parser->object_head);
    parser_begin_scope(parser);

    parser_consume(parser, Token_Left_Parenthesis, "Expect a '(' after the function name.");
    if (!(parser->token_current.kind == Token_Right_Parenthesis)) {
        do {
            parser->compiler->function->arity += 1;
            if (parser->compiler->function->arity > 255)
                parser_error(parser, &parser->token_current, "Can't have more than 255 parameters.");

            uint8_t indentifier_index = parser_parse_identifier(parser, "Expect parameter name.");

            // Define Identifier
            //  
            if (parser->compiler->depth == 0) {
                Compiler_CompileInstruction_2Bytes(
                    parser_get_current_bytecode(parser),
                    OpCode_Define_Global,
                    indentifier_index,
                    parser->token_previous.line_number
                );
            } else {
                parser_initialize_local_identifier(parser);
            }
        } while (parser_match_then_advance(parser, Token_Comma));
    }
    parser_consume(parser, Token_Right_Parenthesis, "Expect a ')' after parameters.");
    parser_consume(parser, Token_Left_Brace, "Expect a '{' after parameters.");

    Statement* statement_block = parser_parse_instruction_block(parser, BlockType_Function);

    ObjectFunction* function = Compiler_end(&compiler, &parser->compiler, parser->had_error, parser->token_previous.line_number);

    Compiler_CompileInstruction_Closure(
        parser_get_current_bytecode(parser),
        value_make_object(function),
        parser->token_previous.line_number
    );

    for (int i = 0; i < function->variable_dependencies_count; i++) {
        Compiler_CompileInstruction_1Byte(
            parser_get_current_bytecode(parser),
            compiler.variable_dependencies.items[i].location,
            parser->token_previous.line_number
        );
        Compiler_CompileInstruction_1Byte(
            parser_get_current_bytecode(parser),
            (uint8_t)compiler.variable_dependencies.items[i].index,
            parser->token_previous.line_number
        );
    }

    return function;

}

static Statement* parser_parse_declaration_variable(Parser* parser) {
    Statement statement = { 0 };
    statement.kind = StatementKind_Variable_Declaration;

    parser_consume(parser, Token_Identifier, "Expect variable name.");

    block{
        if (parser->compiler->depth == 0)
            break;

        if (StackLocal_is_full(&parser->compiler->locals)) {
            parser_error(parser, &parser->token_previous, "Too many local variables in a scope.");
            break;
        }

        if (parser_check_locals_duplicates(parser, &parser->token_previous)) {
            parser_error(parser, &parser->token_previous, "Already a variable with this name in this scope.");
            return statement_allocate(statement);
        }

        StackLocal_push(
            &parser->compiler->locals,
            parser->token_previous,
            -1, // Mark Local as Uninitialized 
            false
        );

        statement.variable_declaration.identifier = ObjectString_allocate_if_not_interned(parser->string_database, parser->token_previous.start, parser->token_previous.length, parser->object_head);

        // Check for assignment
        // 
        if (parser_match_then_advance(parser, Token_Equal)) {
            statement.variable_declaration.rhs = parser_parse_expression(parser, OperatorPrecedence_Assignment);
        } else {
            Compiler_CompileInstruction_1Byte(
                parser_get_current_bytecode(parser),
                OpCode_Nil,
                parser->token_previous.line_number
            );

            statement.variable_declaration.rhs = expression_allocate(expression_make_nil());
        }

        parser_consume(parser, Token_Semicolon, "Expected ';' after expression.");

        // Mark local as Initialized
        //
        Local* local = &parser->compiler->locals.items[parser->compiler->locals.top - 1];
        local->scope_depth = parser->compiler->depth;

        return statement_allocate(statement);
    }

        //
        // Global Scope Only
        //

    uint8_t global_index = 0;

    statement.variable_declaration.identifier = ObjectString_allocate_if_not_interned(parser->string_database, parser->token_previous.start, parser->token_previous.length, parser->object_head);
    Value value_string = value_make_object_string(statement.variable_declaration.identifier);
    int value_index = Compiler_CompileValue(parser_get_current_bytecode(parser), value_string);
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
        Compiler_CompileInstruction_1Byte(parser_get_current_bytecode(parser), OpCode_Nil, parser->token_previous.line_number);
        statement.variable_declaration.rhs = expression_allocate(expression_make_nil());
    }

    parser_consume(parser, Token_Semicolon, "Expected ';' after expression.");

    // Define Global Varible
    //
    Compiler_CompileInstruction_2Bytes(
        parser_get_current_bytecode(parser),
        OpCode_Define_Global,
        global_index,
        parser->token_previous.line_number
    );

    return statement_allocate(statement);
}

static uint8_t parser_store_identifier_into_bytecode(Parser* parser, const char* start, int length) {
    ObjectString* object_string = ObjectString_allocate_if_not_interned(parser->string_database, start, length, parser->object_head);
    Value value_string = value_make_object_string(object_string);
    int value_index = Compiler_CompileValue(parser_get_current_bytecode(parser), value_string);
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
        // TODO: case: Token_Module:
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
        // TODO: case Token_Left_Bracket: -- Array Subscript
        OperatorMetadata operator_metadata = { 0 };
        operator_metadata.precedence = OperatorPrecedence_Grouping_FunctionCall_And_ObjectGet;
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
        } else if (parser->token_previous.kind == Token_Left_Parenthesis) {
            expression = parser_parse_operator_function_call(parser, expression);
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

        Compiler_CompileInstruction_Constant(parser_get_current_bytecode(parser), number, parser->token_previous.line_number);

        return expression_allocate(e_number);
    }

    if (parser->token_previous.kind == Token_Verdadi) {
        Expression e_true = expression_make_boolean(true);

        Compiler_CompileInstruction_1Byte(parser_get_current_bytecode(parser), OpCode_True, parser->token_previous.line_number);
        return expression_allocate(e_true);
    }

    if (parser->token_previous.kind == Token_Falsu) {
        Expression e_false = expression_make_boolean(false);

        Compiler_CompileInstruction_1Byte(parser_get_current_bytecode(parser), OpCode_False, parser->token_previous.line_number);
        return expression_allocate(e_false);
    }

    if (parser->token_previous.kind == Token_Nulo) {
        Expression nil = expression_make_nil();

        Compiler_CompileInstruction_1Byte(parser_get_current_bytecode(parser), OpCode_Nil, parser->token_previous.line_number);
        return expression_allocate(nil);
    }

    if (parser->token_previous.kind == Token_String) {
        // Check if the source_string already exists in the global string
        // database in the virtual machine. If it does, reuse it, if not
        // allocate a new one and store it in the global string database for
        // future reference.
        //
        ObjectString* string = ObjectString_allocate_if_not_interned(parser->string_database, parser->token_previous.start + 1, parser->token_previous.length - 2, parser->object_head);

        Value v_string = value_make_object(string);
        Expression e_string = expression_make_string(string);

        Compiler_CompileInstruction_Constant(parser_get_current_bytecode(parser), v_string, parser->token_previous.line_number);
        return expression_allocate(e_string);
    }

    if (parser->token_previous.kind == Token_String_Interpolation) {
        for (;;)
        {
            if (parser->token_previous.kind != Token_String_Interpolation)
                break;

            ObjectString* string = ObjectString_allocate_if_not_interned(parser->string_database, parser->token_previous.start + 1, parser->token_previous.length - 2, parser->object_head);
            Value v_string = value_make_object(string);
            Expression e_string = expression_make_string(string);

            // Tracks on many values have it pushed to the stack on runtime.
            //
            parser->interpolation_count_value_pushed += 1;
            Compiler_CompileInstruction_Constant(parser_get_current_bytecode(parser), v_string, parser->token_previous.line_number);

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
            Compiler_CompileInstruction_2Bytes(
                parser_get_current_bytecode(parser),
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

        Compiler_CompileInstruction_1Byte(parser_get_current_bytecode(parser), OpCode_Negation, parser->token_previous.line_number);
        return expression_allocate(negation);
    }

    if (parser->token_previous.kind == Token_Ka) {
        Expression* expression = parser_parse_expression(parser, OperatorPrecedence_Not);
        Expression not = expression_make_not(expression);

        Compiler_CompileInstruction_1Byte(parser_get_current_bytecode(parser), OpCode_Not, parser->token_previous.line_number);
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
        int operand = -1;

        ObjectString* variable_name = ObjectString_allocate_if_not_interned(parser->string_database, parser->token_previous.start, parser->token_previous.length, parser->object_head);

        operand = StackLocal_get_local_index_by_token(&parser->compiler->locals, &parser->token_previous, &local_found);
        if (operand != -1) {
            opcode_assign = OpCode_Assign_Local;
            opcode_read = OpCode_Read_Local;
        } else if ((operand = Compiler_resolve_variable_dependencies(parser->compiler, &parser->token_previous, &local_found)) != -1) {
            // ?????
            // Compiler_resolve_upvalaues return: 
            //     -1: item not found
            //     -2: error too many closure variables in function
            // operand;
            opcode_assign = OpCode_Assign_Heap_Value;
            opcode_read = OpCode_Read_Heap_Value;
        } else {
            opcode_assign = OpCode_Assign_Global;
            opcode_read = OpCode_Read_Global;

            Value value_string = value_make_object_string(variable_name);
            int value_index = Compiler_CompileValue(parser_get_current_bytecode(parser), value_string);
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
            Compiler_CompileInstruction_2Bytes(parser_get_current_bytecode(parser), opcode_assign, operand, parser->token_previous.line_number);

            return expression_allocate(expression);
        }

        // Get variable value
        //
        Compiler_CompileInstruction_2Bytes(
            parser_get_current_bytecode(parser),
            opcode_read,
            operand,
            parser->token_previous.line_number
        );

        Expression expression = expression_make_variable(variable_name);
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
    return (
        token_kind == Token_Equal_Equal ||
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
        int operand_index = Compiler_CompileInstruction_Jump(parser_get_current_bytecode(parser), OpCode_Jump_If_False, parser->token_previous.line_number);
        Compiler_CompileInstruction_1Byte(parser_get_current_bytecode(parser), OpCode_Pop, parser->token_previous.line_number);

        Expression* rigth_operand = parser_parse_expression(parser, OperatorPrecedence_And);

        Compiler_PatchInstructionJump(parser_get_current_bytecode(parser), operand_index);

        Expression expression_and = expression_make_and(left_operand, rigth_operand);
        return expression_allocate(expression_and);
    } break;
    case Token_Ou: {
        int jump_if_false_operand_index = Compiler_CompileInstruction_Jump(parser_get_current_bytecode(parser), OpCode_Jump_If_False, parser->token_previous.line_number);
        int jump_operand_index = Compiler_CompileInstruction_Jump(parser_get_current_bytecode(parser), OpCode_Jump, parser->token_previous.line_number);

        Compiler_PatchInstructionJump(parser_get_current_bytecode(parser), jump_if_false_operand_index);
        Compiler_CompileInstruction_1Byte(parser_get_current_bytecode(parser), OpCode_Pop, parser->token_previous.line_number);

        Expression* rigth_operand = parser_parse_expression(parser, OperatorPrecedence_Or);
        Compiler_PatchInstructionJump(parser_get_current_bytecode(parser), jump_operand_index);

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
        Compiler_CompileInstruction_1Byte(parser_get_current_bytecode(parser), OpCode_Addition, parser->token_previous.line_number);
        Expression addition = expression_make_addition(left_operand, right_operand);
        return expression_allocate(addition);
    } break;
    case Token_Minus: {
        Compiler_CompileInstruction_1Byte(parser_get_current_bytecode(parser), OpCode_Subtraction, parser->token_previous.line_number);
        Expression subtraction = expression_make_subtraction(left_operand, right_operand);
        return expression_allocate(subtraction);
    } break;
    case Token_Asterisk: {
        Compiler_CompileInstruction_1Byte(parser_get_current_bytecode(parser), OpCode_Multiplication, parser->token_previous.line_number);
        Expression multiplication = expression_make_multiplication(left_operand, right_operand);
        return expression_allocate(multiplication);
    } break;
    case Token_Slash: {
        Compiler_CompileInstruction_1Byte(parser_get_current_bytecode(parser), OpCode_Division, parser->token_previous.line_number);
        Expression division = expression_make_division(left_operand, right_operand);
        return expression_allocate(division);
    } break;
    case Token_Caret: {
        Compiler_CompileInstruction_1Byte(parser_get_current_bytecode(parser), OpCode_Exponentiation, parser->token_previous.line_number);
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
        Compiler_CompileInstruction_1Byte(parser_get_current_bytecode(parser), OpCode_Equal_To, parser->token_previous.line_number);
        Expression equal_to = expression_make_equal_to(left_operand, right_operand);
        return expression_allocate(equal_to);
    } break;
    case Token_Not_Equal: {
        // a != b has the same semantics as !(a == b)
        //
        Compiler_CompileInstruction_1Byte(parser_get_current_bytecode(parser), OpCode_Equal_To, parser->token_previous.line_number);
        Compiler_CompileInstruction_1Byte(parser_get_current_bytecode(parser), OpCode_Not, parser->token_previous.line_number);

        Expression* equal_to = expression_allocate(expression_make_equal_to(left_operand, right_operand));
        return expression_allocate(expression_make_not(equal_to));
    } break;
    case Token_Greater: {
        Compiler_CompileInstruction_1Byte(parser_get_current_bytecode(parser), OpCode_Greater_Than, parser->token_previous.line_number);

        Expression greater_than = expression_make_greater_than(left_operand, right_operand);
        return expression_allocate(greater_than);
    } break;
    case Token_Greater_Equal: {
        // a >= b has the same semantics as !(a < b)
        //
        Compiler_CompileInstruction_1Byte(parser_get_current_bytecode(parser), OpCode_Less_Than, parser->token_previous.line_number);
        Compiler_CompileInstruction_1Byte(parser_get_current_bytecode(parser), OpCode_Not, parser->token_previous.line_number);

        Expression* greater_than = expression_allocate(expression_make_greater_than(left_operand, right_operand));
        Expression greater_than_or_equal_to = expression_make_not(greater_than);
        return expression_allocate(greater_than_or_equal_to);
    } break;
    case Token_Less: {
        Compiler_CompileInstruction_1Byte(parser_get_current_bytecode(parser), OpCode_Less_Than, parser->token_previous.line_number);

        Expression less_than = expression_make_less_than(left_operand, right_operand);
        return expression_allocate(less_than);
    } break;
    case Token_Less_Equal: {
        // a <= b has the same semantics as !(a > b)
        //
        Compiler_CompileInstruction_1Byte(parser_get_current_bytecode(parser), OpCode_Greater_Than, parser->token_previous.line_number);
        Compiler_CompileInstruction_1Byte(parser_get_current_bytecode(parser), OpCode_Not, parser->token_previous.line_number);

        Expression less_than_or_equal_to = expression_make_less_than_or_equal_to(left_operand, right_operand);
        return expression_allocate(less_than_or_equal_to);
    } break;
    }

    assert(false && "Error: Unhandled Relational Operator.");
    return NULL;
}

static uint8_t parser_parse_arguments(Parser* parser) {
    uint8_t argument_count = 0;
    if (parser->token_current.kind != Token_Right_Parenthesis) {
        do {
            Expression* expression = parser_parse_expression(parser, OperatorPrecedence_Assignment);
            if (argument_count == 255) {
                parser_error(parser, &parser->token_previous, "Can't have more than 255 arguments.");
            }
            argument_count += 1;
        } while (parser_match_then_advance(parser, Token_Comma));
    }
    parser_consume(parser, Token_Right_Parenthesis, "Expect ')' after arguments");
    return argument_count;
}

static Expression* parser_parse_operator_function_call(Parser* parser, Expression* left_operand) {
    uint8_t argument_count = parser_parse_arguments(parser);
    Compiler_CompileInstruction_2Bytes(
        parser_get_current_bytecode(parser),
        OpCode_Function_Call, // OpCode
        argument_count,       // Operand
        parser->token_previous.line_number
    );
    return NULL;
}

static Expression* parser_parse_binary(Parser* parser, Expression* left_operand) {
    TokenKind operator_kind_previous = parser->token_previous.kind;

    // 
    // Logical Operations: 
    //     and, or, not 
    //

    if (operator_kind_previous == Token_E) {
        int operand_index = Compiler_CompileInstruction_Jump(parser_get_current_bytecode(parser), OpCode_Jump_If_False, parser->token_previous.line_number);
        Compiler_CompileInstruction_1Byte(parser_get_current_bytecode(parser), OpCode_Pop, parser->token_previous.line_number);

        Expression* expression = parser_parse_expression(parser, OperatorPrecedence_And);

        Compiler_PatchInstructionJump(parser_get_current_bytecode(parser), operand_index);

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
        Compiler_CompileInstruction_1Byte(parser_get_current_bytecode(parser), OpCode_Addition, parser->token_previous.line_number);

        Expression addition = expression_make_addition(left_operand, right_operand);
        return expression_allocate(addition);
    }

    if (operator_kind_previous == Token_Minus)
    {
        Compiler_CompileInstruction_1Byte(parser_get_current_bytecode(parser), OpCode_Subtraction, parser->token_previous.line_number);

        Expression subtraction = expression_make_subtraction(left_operand, right_operand);
        return expression_allocate(subtraction);
    }

    if (operator_kind_previous == Token_Asterisk)
    {
        Compiler_CompileInstruction_1Byte(parser_get_current_bytecode(parser), OpCode_Multiplication, parser->token_previous.line_number);

        Expression multiplication = expression_make_multiplication(left_operand, right_operand);
        return expression_allocate(multiplication);
    }

    if (operator_kind_previous == Token_Slash)
    {
        Compiler_CompileInstruction_1Byte(parser_get_current_bytecode(parser), OpCode_Division, parser->token_previous.line_number);

        Expression division = expression_make_division(left_operand, right_operand);
        return expression_allocate(division);
    }

    if (operator_kind_previous == Token_Caret)
    {
        Compiler_CompileInstruction_1Byte(parser_get_current_bytecode(parser), OpCode_Exponentiation, parser->token_previous.line_number);

        Expression exponentiation = expression_make_division(left_operand, right_operand);
        return expression_allocate(exponentiation);
    }

    //
    // Comparison or Relational Operations
    // 

    if (operator_kind_previous == Token_Equal_Equal)
    {
        Compiler_CompileInstruction_1Byte(parser_get_current_bytecode(parser), OpCode_Equal_To, parser->token_previous.line_number);

        Expression equal_to = expression_make_equal_to(left_operand, right_operand);
        return expression_allocate(equal_to);
    }

    if (operator_kind_previous == Token_Not_Equal)
    {
        // a != b has the same semantics as !(a == b)
        //
        Compiler_CompileInstruction_1Byte(parser_get_current_bytecode(parser), OpCode_Equal_To, parser->token_previous.line_number);
        Compiler_CompileInstruction_1Byte(parser_get_current_bytecode(parser), OpCode_Not, parser->token_previous.line_number);

        Expression* equal_to = expression_allocate(expression_make_equal_to(left_operand, right_operand));
        return expression_allocate(expression_make_not(equal_to));
    }

    if (operator_kind_previous == Token_Greater)
    {
        Compiler_CompileInstruction_1Byte(parser_get_current_bytecode(parser), OpCode_Greater_Than, parser->token_previous.line_number);

        Expression greater_than = expression_make_greater_than(left_operand, right_operand);
        return expression_allocate(greater_than);
    }

    if (operator_kind_previous == Token_Greater_Equal)
    {
        // a >= b has the same semantics as !(a < b)
        //
        Compiler_CompileInstruction_1Byte(parser_get_current_bytecode(parser), OpCode_Less_Than, parser->token_previous.line_number);
        Compiler_CompileInstruction_1Byte(parser_get_current_bytecode(parser), OpCode_Not, parser->token_previous.line_number);

        Expression* greater_than = expression_allocate(expression_make_greater_than(left_operand, right_operand));
        Expression greater_than_or_equal_to = expression_make_not(greater_than);
        return expression_allocate(greater_than_or_equal_to);
    }

    if (operator_kind_previous == Token_Less)
    {
        Compiler_CompileInstruction_1Byte(parser_get_current_bytecode(parser), OpCode_Less_Than, parser->token_previous.line_number);

        Expression less_than = expression_make_less_than(left_operand, right_operand);
        return expression_allocate(less_than);
    }

    if (operator_kind_previous == Token_Less_Equal)
    {
        // a <= b has the same semantics as !(a > b)
        //
        Compiler_CompileInstruction_1Byte(parser_get_current_bytecode(parser), OpCode_Greater_Than, parser->token_previous.line_number);
        Compiler_CompileInstruction_1Byte(parser_get_current_bytecode(parser), OpCode_Not, parser->token_previous.line_number);

        Expression* less_than = expression_allocate(expression_make_less_than(left_operand, right_operand));
        Expression less_than_or_equal_to = expression_make_not(less_than);
        return expression_allocate(less_than_or_equal_to);
    }

    // TODO: log token name
    assert(false && "Unhandled token...");

    return NULL;
}

static bool parser_check_locals_duplicates(Parser* parser, Token* identifier) {
    StackLocal* locals = &parser->compiler->locals;
    for (int i = locals->top - 1; i >= 0; i--) {
        Local* local = &locals->items[i];
        if (local->scope_depth < parser->compiler->depth &&
            local->scope_depth != -1) {
            break;
        }

        if (token_is_identifier_equal(identifier, &local->token))
            return true;
    }

    return false;
}

// TODO: remove
// static void parser_end_parsing(Parser* parser) {
//     Compiler_CompileInstruction_1Byte(parser_get_current_bytecode(parser), OpCode_Return, parser->token_current.line_number);
// }

static void parser_begin_scope(Parser* parser) {
    // parser->scope.depth += 1;
    parser->compiler->depth += 1;
}

static void parser_end_scope(Parser* parser) {
    parser->compiler->depth -= 1;

    StackLocal* locals = &parser->compiler->locals;
    for (;;) {
        // if (locals->top <= 0) break;
        // if (locals->items[locals->top - 1].scope_depth <= parser->compiler->depth) break;

        if (StackLocal_is_empty(locals)) break;
        if (StackLocal_peek(locals, 0)->scope_depth <= parser->compiler->depth) break;

        if (StackLocal_peek(locals, 0)->action == LocalAction_Move_Heap) {
            Compiler_CompileInstruction_1Byte(
                parser_get_current_bytecode(parser),
                OpCode_Move_To_Heap,
                parser->token_previous.line_number
            );
        } else {
            Compiler_CompileInstruction_1Byte(
                parser_get_current_bytecode(parser),
                OpCode_Pop,
                parser->token_previous.line_number
            );
        }

        StackLocal_pop(locals);
    }
}

// todo: parser destroy implementation