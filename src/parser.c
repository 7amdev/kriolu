#include "kriolu.h"

static void parser_advance(Parser* parser);
static void parser_consume(Parser* parser, TokenKind kind, const char* error_message, ...);
static bool parser_match_then_advance(Parser* parser, TokenKind kind);
static void parser_synchronize(Parser* parser);
static void parser_error(Parser* parser, Token* token, const char* message, ...);
static Bytecode* parser_get_current_bytecode(Parser* parser);
static Statement* parser_parse_statement(Parser* parser, BlockType block_type);
static Statement* parser_parse_instruction_print(Parser* parser);
static Statement* parser_parse_instruction_block(Parser* parser, BlockType is_pure_block);
static Statement* parser_parse_instruction_if(Parser* parser);
static Statement* parser_instruction_while(Parser* parser);
static Statement* parser_instruction_for(Parser* parser);
static Statement* parser_instruction_break(Parser* parser);
static Statement* parser_parse_instruction_debug(Parser* parser);
static Statement* parser_instruction_continue(Parser* parser);
static Statement* parser_instruction_return(Parser* parser);
static Statement* parser_parse_declaration_class(Parser* parser);
static Statement* parser_parse_declaration_function(Parser* parser);
static Statement* parser_parse_declaration_variable(Parser* parser);
static int parser_find_identifier_location(Parser* parser, Token token, int* out_identifier_location, Local* out_local);
static void Parser_compile_variable_value_to_stack(Parser* parser, int identifier_location, int identifier_location_index);
static ObjectString* parser_intern_token(Token token, Object** object_head, HashTable* string_database);
static int parser_save_identifier_into_bytecode(Bytecode* bytecode, ObjectString* identifier);
static void parser_initialize_local_identifier(Parser* parser);
static void parser_load_variable_value_to_stack(Parser* parser, Token variable_name);
static ObjectFunction* parser_parse_function_paramenters_and_body(Parser* parser, FunctionKind function_kind, Token class_name);
static Statement* parser_parse_statement_expression(Parser* parser); // TODO: rename to ???
static Expression* parser_parse_expression(Parser* parser, OperatorPrecedence operator_precedence_previous);
static Expression* parser_parse_literals(Parser* parser, bool can_assign);
static Expression* parser_parse_operator_function_call(Parser* parser, Expression* left_operand);
static Expression* parser_parse_operator_object_getter_and_setter(Parser* parser, Expression* left_expression, bool can_assign);
static void parser_parse_operator_assignment(Parser* parser, int identifier_location, int identifier_location_index);
static Expression* parser_parse_operators_unary(Parser* parser);
static Expression* parser_parse_operators_logical(Parser* parser, Expression* left_operand);
static Expression* parser_parse_operators_arithmetic(Parser* parser, Expression* left_operand);
static Expression* parser_parse_operators_relational(Parser* parser, Expression* left_operand);
static uint8_t parser_parse_arguments(Parser* parser, TokenKind token_kind);
// TODO: rename to Function_find_local_by_token(Function* function, Token* name, Local** out);
static void parser_compile_return(Parser* parser);
static bool parser_is_operator_arithmetic(Token token);
static bool parser_is_operator_logical(Token token);
static bool parser_is_operator_relational(Token token);
static bool parser_is_operator_assignment(Token token);
static bool parser_is_operator_unary(Token token);
static bool parser_is_literals(Token token);
static bool parser_is_identifier_a_superclass(Function* first_function, Token identifier);
static bool parser_check_locals_duplicates(Parser* parser, Token* identifier);
static void parser_end_parsing(Parser* parser);
static void parser_begin_scope(Parser* parser);
static void parser_end_scope(Parser* parser);

static void parser_init_function(Parser* parser, Function* function, FunctionKind function_kind, Token class_name);
static ObjectFunction* parser_end_function(Parser* parser, Function* function);

void parser_init(Parser* parser, const char* source_code, ParserInitParams params) {
    parser->token_current = (Token){ 0 };  // token_error
    parser->token_previous = (Token){ 0 }; // token_error
    parser->panic_mode = false;
    parser->had_error = false;
    parser->lexer = params.lexer;
    if (params.lexer == NULL) {
        parser->lexer = lexer_create_static();
        assert(parser->lexer);
        lexer_init(parser->lexer, source_code);
    }
    parser->function = NULL;
    parser->first_class_declaration = NULL;
    parser->interpolation_count_nesting = 0;
    parser->interpolation_count_value_pushed = 0;
    parser->continue_jump_to = -1;

    hash_table_init(&parser->table_strings);
    parser->string_database = params.string_database;
    if (parser->string_database == NULL) {
        parser->string_database = &parser->table_strings;
    }

    parser->objects = NULL;
    parser->object_head = params.object_head;
    if (parser->object_head == NULL)  
        parser->object_head = &parser->objects;

    StackBreak_init(&parser->breakpoints);
    StackBlock_init(&parser->blocks);

    Memory_register(NULL, NULL, parser);

    parser->debugger_execution_pause  = false;
    parser->debugger_execution_resume = false;
    parser->debugger_functions = (DArrayFunction) {0};
}

ObjectFunction* parser_parse(Parser* parser, ArrayStatement** return_statements, HashTable* string_database, Object** object_head) {
    Function function;
    ArrayStatement* statements = array_statement_allocate();

    parser_advance(parser);
    parser_init_function(parser, &function, FunctionKind_Script, (Token){0});

    for (;;) {
        if (parser->token_current.kind == Token_Eof) break;

        Statement* statement = parser_parse_statement(parser, BlockType_Clean);
        if (statement == NULL) continue;

        array_statement_insert(statements, *statement);
        // TODO: free statement here
    }

    ObjectFunction* compiled_script = parser_end_function(parser, &function);

    if (return_statements != NULL) *return_statements =  statements;
    if (string_database   != NULL) *string_database   = *parser->string_database;
    if (object_head       != NULL) *object_head       =  parser->objects;

    return (
        parser->had_error 
        ? NULL 
        : compiled_script
    );
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

static bool Parser_read_commands(Parser* parser, char** out_error_msg) {
    char input_line[1024]; 

    printf("\n");
    for (;;) {
        DynamicArray_foreach(Function, &parser->debugger_functions, fn) {
            if  (fn.item->object->name == NULL) {
                printf("script");
            } 
            else if (fn.item->function_kind == FunctionKind_Method || fn.item->function_kind == FunctionKind_Method_Initializer) {
                printf("kl_%.*s", fn.item->class_name.length, fn.item->class_name.start);
                printf(" > fn_%.*s",fn.item->object->name->length, fn.item->object->name->characters);
            }
            else {
                printf("%.*s",fn.item->object->name->length, fn.item->object->name->characters);
            } 

            if (fn.i < parser->debugger_functions.count - 1) printf(" > ");
        }

        // LinkedList_foreach(Function, parser->function, it) {
        //     Function* fn = it.curr;
        //     if  (fn->object->name == NULL) {
        //         printf("script");
        //     } 
        //     else if (fn->function_kind == FunctionKind_Method || fn->function_kind == FunctionKind_Method_Initializer) {
        //         printf("fn_%.*s",fn->object->name->length, fn->object->name->characters);
        //         printf(" > kl_%.*s", fn->class_name.length, fn->class_name.start);
        //     }
        //     else {
        //         printf("%.*s",fn->object->name->length, fn->object->name->characters);
        //     } 
        //
        //     if (it.next)
        //         printf(" > ");
        // }

        if ((*parser->function).next == NULL) {
            LinkedList_foreach(ClassDeclaration, parser->first_class_declaration, it) {
                ClassDeclaration* klass = it.curr;
                printf(" > kl_%.*s", klass->name.length, klass->name.start);
                if (it.next) printf(" > ");
            }
        } 

        printf("> $");
        if (fgets(input_line, sizeof(input_line), stdin) == NULL) {
           *out_error_msg = "Cannot read command.";
           return false;
        }

        int input_line_length = (int)strlen(input_line);
        int new_line_idx   = input_line_length - 1;
        if (input_line[new_line_idx] == '\n') {
            input_line[new_line_idx] = '\0';
            input_line_length -= 1;
        } 

        char input[50];
        Debugger_trim_string(input_line, input_line_length, input, sizeof(input));

        if (strcmp(input, "next") == 0) {
            break;
        }
        else if (strcmp(input, "resume") == 0) {
            parser->debugger_execution_resume = true;
            break;
        }
        else if (strcmp(input, "continue") == 0) {
            parser->debugger_execution_pause = false;
            break;
        }
        else if (strcmp(input, "locals") == 0) { 
            StackLocal* locals = &parser->function->locals;
            printf("Stack: ");
            for (int i = 0; i < locals->top; i++) {
               Token token = locals->items[i].token;
               printf("[ %d:", i);
               printf("%.*s", token.length, token.start);
               printf(" ]");
            }
            printf("\n");
        }
        else if (strcmp(input, "vm_stack") == 0) {
//          TODO: shows the locals plus the caller(s) StackeValues
            printf("Input: '%s'\n", input);
        }
        else if (strcmp(input, "closures") == 0) {
//          TODO: display closed values inside the current function
            printf("Input: '%s'\n", input);
        }
        else {
            printf("[INFO] Command '%s' is invalid!\n", input);
        }
    }

    printf("\n");
    return true;
}

static void parser_consume(Parser* parser, TokenKind kind, const char* error_message, ...) {
    va_list args;
    va_start(args, error_message);

    char input_line[1024];

    if (parser->token_current.kind == Token_Debugger_Break) {
        parser_advance(parser);
        Compiler_CompileInstruction_1Byte(
            parser_get_current_bytecode(parser),
            OpCode_Debugger_Break,
            parser->token_previous.line_number
        );

        if (parser->debugger_execution_resume == false) {
            parser->debugger_execution_pause = true;

            char *error_msg = NULL;
            bool ok = Parser_read_commands(parser, &error_msg);
            if (!ok) printf("[ERROR] %s\n", error_msg);
        }
    }

    if (parser->token_current.kind == kind) {
        parser_advance(parser);
        return;
    }

    parser_error(parser, &parser->token_previous, error_message, args);
    va_end(args);
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

// TODO: make it a variadic function
static void parser_error(Parser* parser, Token* token, const char* message, ...) {
    if (parser->panic_mode) return;
    va_list args;
    va_start(args, message);

    FILE* error_output = stdout;

    parser->panic_mode = true;
    parser->had_error = true;

    fprintf(error_output, "[line %d] Error", token->line_number);
    if (token->kind == Token_Eof) {
        fprintf(error_output, " at end");
    }
    else if (token->kind == Token_Error) {
        fprintf(error_output, " at %.*s\n", token->length, token->start);
    }
    else {
        fprintf(error_output, " at '%.*s'", token->length, token->start);
        fprintf(error_output, " : '");
        fprintf(error_output, message, args);
        fprintf(error_output, "'\n");

        // fprintf(error_output, " : '%s'\n", message);
    }

    va_end(args);
}

static Bytecode* parser_get_current_bytecode(Parser* parser) {
    return &parser->function->object->bytecode;
}

static void Parser_debug_print_expression(Lexer lexer, const char* start) {
    bool syntax_error = true;
    
    Token token = lexer_scan(&lexer);
    for (;;) {
        if (token.kind == Token_Mimoria)   break; // Error
        if (token.kind == Token_Funson)    break; // Error
        if (token.kind == Token_Mimoria)   break; // Error
        if (token.kind == Token_Di)        break; // Error
        if (token.kind == Token_Si)        break; // Error
        if (token.kind == Token_Imprimi)   break; // Error
        if (token.kind == Token_Semicolon) {
            syntax_error = false;
            break;
        } 
    
        token = lexer_scan(&lexer);
    }

    int statement_len = (int)((token.start + token.length) - start);
    if (syntax_error == 0) {
        Bytecode_debug_print("%.*s\n", statement_len, start);
        // printf("%.*s\n", statement_len, start);
    } else {
        printf("[Error] syntax error on line '%d'.\n", token.line_number);
    }
}

static void Parser_debug_print_declaration(Lexer lexer, const char* start) {
    int count_brackets = 0;
    
    Token token = lexer_scan(&lexer);
    for (;;) {
        if (token.kind == Token_Left_Brace) {
            count_brackets += 1; 
        }
        else if (token.kind == Token_Right_Brace) {
            count_brackets -= 1;
            if (count_brackets == 0) break;
        }

        if (count_brackets == 1) {
            if (token.kind == Token_Mimoria) break; // Error
            if (token.kind == Token_Funson)  break; // Error
            if (token.kind == Token_Mimoria) break; // Error
            if (token.kind == Token_Di)      break; // Error
            if (token.kind == Token_Si)      break; // Error
            if (token.kind == Token_Imprimi) break; // Error
        }
    
        token = lexer_scan(&lexer);
    }

    if (count_brackets == 0) {
        int statement_len = (int)((token.start + token.length) - start);
        printf("Statement: '");
        printf("%.*s'\n", statement_len, start);
    } else {
        printf("[Error] syntax error on line '%d'.\n", token.line_number);
    }
}

static Statement* parser_parse_statement(Parser* parser, BlockType block_type) {
//  DECLARATIONS: introduces new identifiers
//    klassi, mimoria, funson
//  INSTRUCTIONS: tells the computer to perform an action
//    imprimi, di, si, divolvi, timenti, block
//  EXPRESSIONS: a calculation that produces a result
//    +, -, /, *, call '(', assignment '=', objectGet '.'
//    variableGet, literals

    Statement* statement = { 0 };
    const char* statement_start = parser->token_current.start;
    int instruction_start_index = parser_get_current_bytecode(parser)->instructions.count;

    if (parser_match_then_advance(parser, Token_Klasi)) {
//  #ifdef DEBUG
        Parser_debug_print_declaration(*parser->lexer, parser->token_previous.start);
//  #endif
        statement = parser_parse_declaration_class(parser); 
    } 
    else if (parser_match_then_advance(parser, Token_Funson)) {
        statement = parser_parse_declaration_function(parser);
    }
    else if (parser_match_then_advance(parser, Token_Mimoria)) {
//  #ifdef DEBUG
        Parser_debug_print_expression(*parser->lexer, parser->token_previous.start);
//  #endif
        statement = parser_parse_declaration_variable(parser);
    }
    else if (parser_match_then_advance(parser, Token_Imprimi)) {
//  #ifdef DEBUG
        Parser_debug_print_expression(*parser->lexer, parser->token_previous.start);
//  #endif
        statement = parser_parse_instruction_print(parser);
    }
    else if (parser_match_then_advance(parser, Token_Si))             statement = parser_parse_instruction_if(parser);                
    else if (parser_match_then_advance(parser, Token_Di))             statement = parser_instruction_for(parser);                     
    else if (parser_match_then_advance(parser, Token_Pa))             statement = parser_instruction_for(parser);                     
    else if (parser_match_then_advance(parser, Token_Sai))            statement = parser_instruction_break(parser);                   
    else if (parser_match_then_advance(parser, Token_Salta))          statement = parser_instruction_continue(parser);                
    else if (parser_match_then_advance(parser, Token_Divolvi)) {
//  #ifdef DEBUG
        Parser_debug_print_expression(*parser->lexer, parser->token_previous.start);
//  #endif
        statement = parser_instruction_return(parser);                  
    }   
    else if (parser_match_then_advance(parser, Token_Left_Brace))     statement = parser_parse_instruction_block(parser, block_type); 
    else if (parser_match_then_advance(parser, Token_Timenti))        statement = parser_instruction_while(parser);                   
    else if (parser_match_then_advance(parser, Token_Debugger_Break)) statement = parser_parse_instruction_debug(parser);
    else {
        Parser_debug_print_expression(*parser->lexer, parser->token_current.start);
        statement = parser_parse_statement_expression(parser);
    }

    if (!parser->debugger_execution_resume && parser->debugger_execution_pause) {
        char *error_msg = NULL;
        bool ok = Parser_read_commands(parser, &error_msg);
        if (!ok) printf("[ERROR] %s\n", error_msg);
    }

    if (parser->panic_mode)
        parser_synchronize(parser);

    return statement;
}

static Statement* parser_parse_statement_expression(Parser* parser) {
    SourceCode source_code = { 
        .source_start               =  parser->token_current.start,
        .source_length              = 0,
        .instruction_start_offset   = parser_get_current_bytecode(parser)->instructions.count
    };

    Expression* expression = parser_parse_expression(parser, OperatorPrecedence_Assignment);
    parser_consume(parser, Token_Semicolon, "Expect ';' after expression.");

    source_code.source_length = (int)(parser->token_current.start - source_code.source_start);
    DynamicArray_append(
        &parser_get_current_bytecode(parser)->source_code, 
        source_code
    );

    Compiler_CompileInstruction_1Byte(
        parser_get_current_bytecode(parser), 
        OpCode_Stack_Pop, 
        parser->token_previous.line_number
    );

    Statement statement = (Statement){
        .kind = StatementKind_Expression,
        .expression = expression
    };

    return statement_allocate(statement);
}

static Statement* parser_parse_instruction_print(Parser* parser) {
    SourceCode source_code = { 
        .source_start               =  parser->token_previous.start,
        .source_length              = 0,
        .instruction_start_offset   = parser_get_current_bytecode(parser)->instructions.count
    };

    Expression* expression = parser_parse_expression(parser, OperatorPrecedence_Assignment);
    parser_consume(parser, Token_Semicolon, "Expect ';' after expression.");


    Compiler_CompileInstruction_1Byte(
        parser_get_current_bytecode(parser), 
        OpCode_Print, 
        parser->token_previous.line_number
    );

    source_code.source_length = (int)(parser->token_current.start - source_code.source_start);
    DynamicArray_append(
        &parser_get_current_bytecode(parser)->source_code, 
        source_code
    );

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

        Compiler_PatchInstructionJump(
            parser_get_current_bytecode(parser), 
            breakpoint.operand_index
        );

        StackBreak_pop(breakpoints);
    }

    StackBlock_pop(blocks);
}
static Statement* parser_parse_instruction_block(Parser* parser, BlockType block_type) {
    parser_begin_scope(parser);
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

    parser_end_scope(parser);
    
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
    Compiler_CompileInstruction_1Byte(parser_get_current_bytecode(parser), OpCode_Stack_Pop, parser->token_previous.line_number);

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
    Compiler_CompileInstruction_1Byte(parser_get_current_bytecode(parser), OpCode_Stack_Pop, parser->token_previous.line_number);

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
    Compiler_CompileInstruction_1Byte(parser_get_current_bytecode(parser), OpCode_Stack_Pop, parser->token_previous.line_number);

    Statement* body = parser_parse_statement(parser, false);

    Compiler_CompileInstruction_Loop(parser_get_current_bytecode(parser), loop_start, parser->token_previous.line_number);
    Compiler_PatchInstructionJump(parser_get_current_bytecode(parser), jump_if_false_operand_index);
    Compiler_CompileInstruction_1Byte(parser_get_current_bytecode(parser), OpCode_Stack_Pop, parser->token_previous.line_number);

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
    // 1: Feature::Function Closure support {
    Token variable_name = { 0 };
    int variable_stack_idx = -1;
    // 1: }
    if (parser_match_then_advance(parser, Token_Semicolon)) {
        // Do nothing. There is no initializer.
    } else if (parser_match_then_advance(parser, Token_Mimoria)) {
        // 1: {
        variable_name = parser->token_current;
        // 1: }

        initializer = parser_parse_declaration_variable(parser);

        // 1: {
        variable_stack_idx = parser->function->locals.top - 1;
        // 1: }
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
            OpCode_Stack_Pop,
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
            OpCode_Stack_Pop,
            parser->token_previous.line_number
        );

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

    // 1: {
    int new_variable_idx = -1;
    if (variable_stack_idx != -1) {
        // Create a new local variable:
        // 1. Begin a new scope
        // 2. Copy a value to the top of the Stack
        // 3. Push the variable name to StackLocal storage
        // 4. Initialize the local variable
        // 5. End the scope
        // 
        parser_begin_scope(parser);
        Compiler_CompileInstruction_2Bytes(
            parser_get_current_bytecode(parser),
            OpCode_Stack_Copy_From_idx_To_Top, // Read Iniitialized Variable
            (uint8_t)variable_stack_idx,
            parser->token_previous.line_number
        );

        // Create a new local variable
        // '-1' Mark as Uninitialized
        //
        StackLocal_push(&parser->function->locals, (Local) {
            .token = variable_name,
            .scope_depth = -1,
            .action = LocalAction_Default
        });
        StackLocal_initialize_local(&parser->function->locals, parser->function->depth, 0);
        new_variable_idx = parser->function->locals.top - 1;
    }
    // 1: }

#ifdef DEBUG_TRACE_INSTRUCTION
    printf("------ Body ------\n");
#endif

    Statement* body = parser_parse_statement(parser, BlockType_Loop);

#ifdef DEBUG_TRACE_INSTRUCTION
    printf("--------------------------\n");
#endif

    // 1: {
    if (variable_stack_idx != -1) {
        Compiler_CompileInstruction_2Bytes(
            parser_get_current_bytecode(parser),
            OpCode_Stack_Copy_From_idx_To_Top,
            new_variable_idx,
            parser->token_previous.line_number
        );
        Compiler_CompileInstruction_2Bytes(
            parser_get_current_bytecode(parser),
            OpCode_Stack_Copy_Top_To_Idx,
            variable_stack_idx,
            parser->token_previous.line_number
        );
        Compiler_CompileInstruction_1Byte(
            parser_get_current_bytecode(parser),
            OpCode_Stack_Pop,
            parser->token_previous.line_number
        );

        // Generates the instruction to move the variable, acessed by the closure,
        // from the Stack to the Heap.
        //
        parser_end_scope(parser);
    }
    // 1: }


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
            OpCode_Stack_Pop,
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

// TODO: parser_parse_intruction_debugger_break(PARSER|RUNTIME|BOTH);
static Statement* parser_parse_instruction_debug(Parser* parser) {
    // parser_consume(parser, Token_Debugger_Break, "");

    if (parser->debugger_execution_resume == false) 
        parser->debugger_execution_pause = true;

//  TODO: delete code bellow
//  __debugbreak();

    Compiler_CompileInstruction_1Byte(
        parser_get_current_bytecode(parser),
        OpCode_Debugger_Break,
        parser->token_previous.line_number
    );
    // parser_consume(parser, Token_Semicolon, "Expected ';' after token 'sai'.");

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
    if (parser->function->function_kind == FunctionKind_Method_Initializer) {
        Compiler_CompileInstruction_2Bytes(
            parser_get_current_bytecode(parser),
            OpCode_Stack_Copy_From_idx_To_Top,
            0,
            parser->token_previous.line_number
        );
    } else {
        Compiler_CompileInstruction_1Byte(
            parser_get_current_bytecode(parser),
            OpCode_Stack_Push_Literal_Nil,
            parser->token_current.line_number
        );
    }

    Compiler_CompileInstruction_1Byte(
        parser_get_current_bytecode(parser),
        OpCode_Return,
        parser->token_current.line_number
    );
}

static Statement* parser_instruction_return(Parser* parser) {
    if (parser->function->function_kind == FunctionKind_Script)
        parser_error(parser, &parser->token_previous, "Can't return from top-level code.");

    if (parser->token_current.kind == Token_Semicolon) {
        parser_compile_return(parser);
    } else {
        if (parser->function->function_kind == FunctionKind_Method_Initializer) {
            parser_error(parser, &parser->token_previous, "Can't return a value from a 'Konstrutor'");
        }

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

static ObjectString* parser_intern_token(Token token, Object** object_head, HashTable* string_database) {
    String source_string            = string_make(token.start, token.length);
    uint32_t source_string_hash     = string_hash(source_string);
    ObjectString* identifier_string = ObjectString_Allocate(
        .task = (
            AllocateTask_Check_If_Interned  |
            AllocateTask_Copy_String        |
            AllocateTask_Initialize         |
            AllocateTask_Intern
        ),
        .string = source_string,
        .hash   = source_string_hash,
        .first  = object_head,
        .table  = string_database
    );

    return identifier_string;
}

static int parser_save_identifier_into_bytecode(Bytecode* bytecode, ObjectString* identifier) {
    Value value_string = value_make_object_string(identifier);
    Memory_transaction_push(value_string);
// {
    int value_index = ArrayValue_insert(&bytecode->values, value_string);
// }
    Memory_transaction_pop();
    
    if (value_index > UINT8_MAX) 
        return -1; 

    return value_index;
}

// [Return]                  Identifier Location Index
// [out_identifier_location] Where the identifier is located
//
static int parser_find_identifier_location(Parser* parser, Token token, int* out_identifier_location, Local* out_local) {
//  In the Locals (Current function's Value Stack)
    int local_index = StackLocal_get_local_index_by_token(&parser->function->locals, &token, &out_local);
    if (local_index != -1) {
        *out_identifier_location = 1;       
        return local_index;
    } 

//  Walks the Function Linked-List and search for the variable in the function's locals
//
//  In the parent function's locals; Closure Variable
    int parent_fn_local_index = parser_resolve_variable_dependencies(parser->function, &token, &out_local);
    if (parent_fn_local_index != -1) {
        *out_identifier_location = 2;       
        return parent_fn_local_index;
    } 

//  In Bytecode Array of Values
    ObjectString* identifier = parser_intern_token(token, parser->object_head, parser->string_database);
    int value_index = parser_save_identifier_into_bytecode(
        parser_get_current_bytecode(parser), 
        identifier
    );
    *out_identifier_location = 3;           

    return value_index;
}

static void Parser_compile_variable_value_to_stack(Parser* parser, int identifier_location, int identifier_location_index) {
    if (identifier_location == 1) {
//      Its a Local Variable
        Compiler_CompileInstruction_2Bytes(
            parser_get_current_bytecode(parser),
            OpCode_Stack_Copy_From_idx_To_Top,
            identifier_location_index,
            parser->token_previous.line_number
        );
    } else if (identifier_location == 2) {
//      Its a Closure Variable
        Compiler_CompileInstruction_2Bytes(
            parser_get_current_bytecode(parser),
            OpCode_Stack_Copy_From_Heap_To_Top,
            identifier_location_index,
            parser->token_previous.line_number
        );
    } else {
//      Its a Global Variable
        Compiler_CompileInstruction_2Bytes(
            parser_get_current_bytecode(parser),
            OpCode_Read_Global,
            identifier_location_index,
            parser->token_previous.line_number
        );
    }
}

static void parser_parse_operator_assignment(Parser* parser, int identifier_location, int identifier_location_index) {
    parser_parse_expression(parser, OperatorPrecedence_Assignment);

    if (identifier_location == 1) {
//      Its a Local Variable
        Compiler_CompileInstruction_2Bytes(
            parser_get_current_bytecode(parser),
            OpCode_Stack_Copy_Top_To_Idx,
            identifier_location_index,
            parser->token_previous.line_number
        );
    } else if (identifier_location == 2) {
//      Its a Closure Variable
        Compiler_CompileInstruction_2Bytes(
            parser_get_current_bytecode(parser),
            OpCode_Stack_Move_Top_To_Heap,
            identifier_location_index,
            parser->token_previous.line_number
        );
    } else {
//      Its a Global Variable
        Compiler_CompileInstruction_2Bytes(
            parser_get_current_bytecode(parser),
            OpCode_Assign_Global,
            identifier_location_index,
            parser->token_previous.line_number
        );
    }
}

static void Parser_create_local_uninitialized(Parser* parser, Token identifier) {
    if (parser->function->depth == 0) return;
    
    if (StackLocal_is_full(&parser->function->locals)) {
        parser_error(parser, &identifier,  "Too many local variables in a scope.");
    }

    if (parser_check_locals_duplicates(parser, &identifier)) {
        parser_error(parser, &identifier, "Already a variable with this name in this scope.");
    }
    
    StackLocal_push(&parser->function->locals, (Local) {
        .token = identifier,
        .scope_depth = -1,
        .action = LocalAction_Default
    });
}

static void parser_initialize_local_identifier(Parser* parser) {
    if (parser->function->depth == 0) return;

    Local* local = StackLocal_peek(&parser->function->locals, 0);
    local->scope_depth = parser->function->depth;
}

static void parser_load_variable_value_to_stack(Parser* parser, Token variable_name) {
    int identifier_location = -1;
    int identifier_location_index = parser_find_identifier_location(parser, variable_name, &identifier_location, NULL);
    Parser_compile_variable_value_to_stack(parser, identifier_location, identifier_location_index);
}

static Statement* parser_parse_declaration_class(Parser* parser) {
//  #ifdef DEBUG_SHOW_CODE
    SourceCode source_code = { 
        .source_start = parser->token_previous.start,
        .source_length = 0,
        .instruction_start_offset = -1
    };
//  #endif

    parser_consume(parser, Token_Identifier, "Expected a class name.");

    Token class_name = parser->token_previous;
    ObjectString* os_class_name = parser_intern_token(parser->token_previous, parser->object_head, parser->string_database);
    int class_name_index = parser_save_identifier_into_bytecode(parser_get_current_bytecode(parser), os_class_name);

    if (parser->function->depth > 0) 
        Parser_create_local_uninitialized(parser, parser->token_previous);

//  #ifdef DEBUG_SHOW_CODE
    source_code.instruction_start_offset = (
        parser_get_current_bytecode(parser)->instructions.count
    );
//  #endif
    Compiler_CompileInstruction_2Bytes(
        parser_get_current_bytecode(parser),
        OpCode_Class,
        class_name_index,
        parser->token_previous.line_number
    );

    if (parser->function->depth == 0) {
        Compiler_CompileInstruction_2Bytes(
            parser_get_current_bytecode(parser),
            OpCode_Define_Global,
            class_name_index,
            parser->token_previous.line_number
        );
    } else {
        parser_initialize_local_identifier(parser);
    }

    ClassDeclaration new_class = {0};
    new_class.name = class_name;
    LinkedList_push(parser->first_class_declaration, &new_class);

    if (parser_match_then_advance(parser, Token_Less)) {
        parser_consume(parser, Token_Identifier, "Expect Superclass name.");
        new_class.has_superclass = true;

//      { 
        parser_load_variable_value_to_stack(parser, parser->token_previous);
        if (token_is_identifier_equal(&parser->token_previous, &class_name)) parser_error(parser, &parser->token_previous, "A class cannot inherit from itself.");
        parser_begin_scope(parser); // Ensures there is no collition for multiple class inheriting the same superclass
        Parser_create_local_uninitialized(parser, (Token) {
            .kind   = Token_Super_Class,
            .start  = parser->token_previous.start,
            .length = parser->token_previous.length
        });
        parser_initialize_local_identifier(parser);
//      }         

        parser_load_variable_value_to_stack(parser, class_name);

//      I dont create a new local to sync Parser dummy locals with
//      the VM stack, because the instruction 'OpCode_Inheritance'
//      will Pop the Class Value right after.
        Compiler_CompileInstruction_1Byte(
            parser_get_current_bytecode(parser),
            OpCode_Inheritance,
            parser->token_previous.line_number
        );
    }

    parser_load_variable_value_to_stack(parser, class_name);

    parser_consume(parser, Token_Left_Brace,  "Expected '{' before class body.");
    for (;;) {
        if (parser->token_current.kind == Token_Right_Brace) break;
        if (parser->token_current.kind == Token_Eof)         break;
    
        //
        // Parse Method Declaration
        //
        
//      Note: Check if any method name is a reserved Token
        if (parser->token_current.kind == Token_Salta)
            parser_error(parser, &parser->token_previous, "Identifier 'salta' is a Token used by the Language.");
       
        parser_consume(parser, Token_Identifier, "Expect method name.");

        Token method_name = parser->token_previous;
        Bytecode_debug_print("<fn '%.*s()>'\n", method_name.length, method_name.start);
        Bytecode_debug_increase_indentation();
//      { 
        Bytecode* bytecode       = parser_get_current_bytecode(parser);
        ObjectString* identifier = parser_intern_token(parser->token_previous, parser->object_head, parser->string_database);
        uint8_t identifier_index = parser_save_identifier_into_bytecode(bytecode, identifier);
        if (identifier_index == -1) parser_error(parser, &parser->token_current, "Too many constants.");

        FunctionKind function_kind = FunctionKind_Method;
        if (parser->token_previous.length == 10) 
        if(memcmp(parser->token_previous.start, "konstrutor", 10) == 0) {
            function_kind = FunctionKind_Method_Initializer;
        } 
        
        parser_parse_function_paramenters_and_body(parser, function_kind, class_name);

        Compiler_CompileInstruction_2Bytes(
            parser_get_current_bytecode(parser),
            OpCode_Method,
            identifier_index,
            parser->token_previous.line_number
        );
//      }
        Bytecode_debug_decrease_indentation();
        Bytecode_debug_print("<fn '%.*s()>'\n", method_name.length, method_name.start);
    }
    parser_consume(parser, Token_Right_Brace, "Expected '}' after class body.");

    Compiler_CompileInstruction_1Byte(
        parser_get_current_bytecode(parser),
        OpCode_Stack_Pop,
        parser->token_previous.line_number
    );

    if (new_class.has_superclass) {
        parser_end_scope(parser);
    }

    ClassDeclaration class_popped = {0};
    LinkedList_pop(parser->first_class_declaration, &class_popped);

//  #ifdef DEBUG_SHOW_CODE
    source_code.source_length = (int)(parser->token_current.start - source_code.source_start);
    DynamicArray_append(
        &parser_get_current_bytecode(parser)->source_code, 
        source_code
    );
//  #endif
    
    return NULL;
}

static Statement* parser_parse_declaration_function(Parser* parser) {
    Statement statement = { 0 };
    statement.kind = StatementKind_Funson;

    parser_consume(parser, Token_Identifier, "Expect function name identifier.");

    if (parser->function->depth == 0) { 
        ObjectString* identifier = parser_intern_token(parser->token_previous, parser->object_head, parser->string_database);
        Bytecode* bytecode = parser_get_current_bytecode(parser);
        int function_name_index = parser_save_identifier_into_bytecode(bytecode, identifier);
        if (function_name_index == -1) parser_error(parser, &parser->token_current, "Too many constants.");

        parser_parse_function_paramenters_and_body(parser, FunctionKind_Function, (Token) {0});
        Compiler_CompileInstruction_2Bytes(
            parser_get_current_bytecode(parser),
            OpCode_Define_Global,
            function_name_index,
            parser->token_previous.line_number
        );
    } else {  
        if (StackLocal_is_full(&parser->function->locals)) {
            parser_error(parser, &parser->token_previous, "Too many local variables in a scope.");
        }

        if (parser_check_locals_duplicates(parser, &parser->token_previous)) {
            parser_error(parser, &parser->token_previous, "Already a variable with this name in this scope.");
        }
       
        StackLocal_push(&parser->function->locals, (Local) {
            .token = parser->token_previous,
            .scope_depth = parser->function->depth,
            .action = LocalAction_Default
        });

        parser_parse_function_paramenters_and_body(
            parser, 
            FunctionKind_Function,
            (Token){0}
        );
    }

    return statement_allocate(statement);
}

static Statement* parser_parse_method_declaration(Parser* parser) {
    return NULL;
}

static ObjectFunction* parser_parse_function_paramenters_and_body(Parser* parser, FunctionKind function_kind, Token class_name) {
    Function function;
    parser_init_function(parser, &function, function_kind, class_name);

//  NOTE: Parsing function doesnt explecitly close the Scope 'parser_end_scope()', because
//        in the 'Parser_end_function' will emit the Return instruction which at runtime will
//        do the exact task parser_end_scope would do and some more. 
    parser_begin_scope(parser);

    parser_consume(parser, Token_Left_Parenthesis, "Expect a '(' after the function name.");
    if (!(parser->token_current.kind == Token_Right_Parenthesis)) {
        do {
            parser_consume(parser, Token_Identifier, "Expect parameter name.");

            parser->function->object->arity += 1;
            if (parser->function->object->arity > 255) parser_error(parser, &parser->token_current, "Can't have more than 255 parameters.");

            if (parser->function->depth == 0) { 
                ObjectString* identifier = parser_intern_token(parser->token_previous, parser->object_head, parser->string_database);
                Bytecode* bytecode = parser_get_current_bytecode(parser);
                int indentifier_index = parser_save_identifier_into_bytecode(bytecode, identifier);
                if (indentifier_index == -1) parser_error(parser, &parser->token_current, "Too many constants.");

                Compiler_CompileInstruction_2Bytes(
                    parser_get_current_bytecode(parser),
                    OpCode_Define_Global,
                    indentifier_index,
                    parser->token_previous.line_number
                );
            } else {  
                if (StackLocal_is_full(&parser->function->locals)) {
                    parser_error(parser, &parser->token_previous, "Too many local variables in a scope.");
                }

                if (parser_check_locals_duplicates(parser, &parser->token_previous)) {
                    parser_error(parser, &parser->token_previous, "Already a variable with this name in this scope.");
                }
                
                StackLocal_push(&parser->function->locals, (Local) {
                    .token = parser->token_previous,
                    .scope_depth = parser->function->depth,
                    .action = LocalAction_Default
                });
            }
        } while (parser_match_then_advance(parser, Token_Comma));
    }
    parser_consume(parser, Token_Right_Parenthesis, "Expect a ')' after parameters.");
    parser_consume(parser, Token_Left_Brace, "Expect a '{' after parameters.");

    Statement* statement_block = parser_parse_instruction_block(parser, BlockType_Function);

    ObjectFunction* object_fn = parser_end_function(parser, &function);

    Memory_transaction_push(value_make_object(object_fn));
    {
        Compiler_CompileInstruction_Closure(
            parser_get_current_bytecode(parser),
            value_make_object(object_fn),
            parser->token_previous.line_number
        );
    
        for (int i = 0; i < object_fn->variable_dependencies_count; i++) {
            // Compile 1(one) Byte Instruction
            //
            Bytecode_insert_instruction_1byte(
                parser_get_current_bytecode(parser),
                function.variable_dependencies.items[i].location,
                parser->token_previous.line_number,
                false
            );
    
            // Compile 1(one) Byte Instruction
            //
            Bytecode_insert_instruction_1byte(
                parser_get_current_bytecode(parser),
                (uint8_t)function.variable_dependencies.items[i].index,
                parser->token_previous.line_number,
                false
            );
        }
    } 
    Memory_transaction_pop();

    return object_fn;
}

static Statement* parser_parse_declaration_variable(Parser* parser) {
    Statement statement = { 0 };
    statement.kind = StatementKind_Variable_Declaration;

    SourceCode source_code = { 
        .source_start               =  parser->token_previous.start,
        .source_length              = 0,
        .instruction_start_offset   = parser_get_current_bytecode(parser)->instructions.count
    };

    parser_consume(parser, Token_Identifier, "Expect variable name.");

    do {
        if (parser->function->depth == 0)
            break;

        if (StackLocal_is_full(&parser->function->locals)) {
            parser_error(parser, &parser->token_previous, "Too many local variables in a scope.");
            break;
        }

        if (parser_check_locals_duplicates(parser, &parser->token_previous)) {
            parser_error(parser, &parser->token_previous, "Already a variable with this name in this scope.");
            return statement_allocate(statement);
        }
        
        StackLocal_push(&parser->function->locals, (Local) {
            .token = parser->token_previous,
            .scope_depth = -1,
            .action = LocalAction_Default
        });

        String source_string = string_make(parser->token_previous.start, parser->token_previous.length);
        uint32_t source_hash = string_hash(source_string);
        statement.variable_declaration.identifier = ObjectString_Allocate(
            .task = (
                AllocateTask_Check_If_Interned |
                AllocateTask_Copy_String       |
                AllocateTask_Initialize        |
                AllocateTask_Intern
            ),
            .string = source_string,
            .hash = source_hash,
            .first = parser->object_head,    // .first = &parser->objects,
            .table = parser->string_database
        );

        // Check for assignment
        // 
        if (parser_match_then_advance(parser, Token_Equal)) {
            statement.variable_declaration.rhs = parser_parse_expression(parser, OperatorPrecedence_Assignment);
        } else {
            Compiler_CompileInstruction_1Byte(
                parser_get_current_bytecode(parser),
                OpCode_Stack_Push_Literal_Nil,
                parser->token_previous.line_number
            );

            statement.variable_declaration.rhs = expression_allocate(expression_make_nil());
        }

        parser_consume(parser, Token_Semicolon, "Expected ';' after expression.");

        // TODO: #ifdef DEBUG_SHOW_CODE
        // 
        source_code.source_length = (int)(parser->token_current.start - source_code.source_start);
        DynamicArray_append(
            &parser_get_current_bytecode(parser)->source_code, 
            source_code
        );
        
        // Mark local as Initialized
        //
        Local* local = &parser->function->locals.items[parser->function->locals.top - 1];
        local->scope_depth = parser->function->depth;

        return statement_allocate(statement);
    } while (0);

    //
    // Global Scope Only
    //

    uint8_t global_index = 0;

    String source_string = string_make(parser->token_previous.start, parser->token_previous.length);
    uint32_t source_hash = string_hash(source_string);
    statement.variable_declaration.identifier = ObjectString_Allocate(
        .task = (
            AllocateTask_Check_If_Interned |
            AllocateTask_Copy_String       |
            AllocateTask_Initialize        |
            AllocateTask_Intern
        ),
        .string = source_string,
        .hash   = source_hash,
        .first  = parser->object_head,   // .first = &parser->objects,
        .table  = parser->string_database
    );

    Value value_string = value_make_object_string(statement.variable_declaration.identifier);
    Memory_transaction_push(value_string);
    {
        int value_index = Compiler_CompileValue(
            parser_get_current_bytecode(parser), 
            value_string
        );

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
            Compiler_CompileInstruction_1Byte(
                parser_get_current_bytecode(parser),
                OpCode_Stack_Push_Literal_Nil,
                parser->token_previous.line_number
            );
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
    }
    Memory_transaction_pop();

    // TODO: #ifdef DEBUG_SHOW_CODE
    // 
    source_code.source_length = (int)(parser->token_current.start - source_code.source_start);
    DynamicArray_append(
        &parser_get_current_bytecode(parser)->source_code, 
        source_code
    );

    return statement_allocate(statement);
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
//      NOTE: Since its a bytecode interpreter, we use right-associativity when parsing 
//            and emitting instructions to make jumps-instructions efficient, beacuse 
//            we now the end of the statement while preserving left-associativity when 
//            interpreting the code. 
//            Otherwise if we were to use a AST interpreter, we would use left-associativity 
//            when parsing to ensure that the interpretation will occour from left-to-right;
        OperatorMetadata operator_metadata = { 0 };
        operator_metadata.precedence = OperatorPrecedence_Or;
        operator_metadata.is_left_associative = false;
        operator_metadata.is_right_associative = true;
        return operator_metadata;
    }
    case Token_E: {
//      NOTE: Since its a bytecode interpreter, we use right-associativity when parsing 
//            and emitting instructions to make jumps-instructions efficient, beacuse 
//            we now the end of the statement while preserving left-associativity when 
//            interpreting the code. 
//            Otherwise if we were to use a AST interpreter, we would use left-associativity 
//            when parsing to ensure that the interpretation will occour from left-to-right;
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
    case Token_Left_Parenthesis:
    case Token_Left_Brace: {
        // TODO: case Token_Left_Bracket: -- Array Subscript
        OperatorMetadata operator_metadata = { 0 };
        operator_metadata.precedence = OperatorPrecedence_Subcript_Call_ObjectGettersSetters_Grouping;
        operator_metadata.is_left_associative = true;
        operator_metadata.is_right_associative = false;
        return operator_metadata;
    }
    }
}

static bool parser_is_identifier_a_superclass(Function* first_function, Token identifier) {
//  TODO: Include scope depth to equality comparision
    LinkedList_foreach(Function, first_function, it) {
        Function fn = *it.curr;
        for (int i = fn.locals.top - 1; i >= 0 ; i--) {
            Local local = {0};
            Stack_peek(&fn.locals, &local, i);
            bool is_equal = (memcmp(identifier.start, local.token.start, identifier.length) == 0);
            if (is_equal && local.token.kind == Token_Super_Class) 
                return true;
        }
    }

    return false;
}

static void parser_parse_superclass_method_call(Parser* parser) {
    Token superclass = parser->token_previous;

    if (parser->first_class_declaration == NULL) {
        parser_error(parser, &parser->token_previous, "Can't use '%.*s' outside of a class.", superclass.length, superclass.start);
    }
    else if (!parser->first_class_declaration->has_superclass) {
        parser_error(parser, &parser->token_previous, "Can't use '%.*s' in a class with no superclass.", superclass.length, superclass.start);
    }

    parser_consume(parser, Token_Dot, "Expect '.' after '%.*s'.", superclass.length, superclass.start);
    parser_consume(parser, Token_Identifier, "Expect superclass method name.");

    int method_location = -1;
    // TODO
    // if location is on current Stack / Local -> print error 
    // Identifier location index must be a method in a parent class
    int method_location_index = parser_find_identifier_location(parser, parser->token_previous, &method_location, NULL);
    parser_load_variable_value_to_stack(parser, (Token) {
        .kind = Token_Keli,
        .start = "keli",
        .length = 4
    });

    if (parser_match_then_advance(parser, Token_Left_Parenthesis)) {
        uint8_t argument_count = parser_parse_arguments(parser, Token_Left_Parenthesis);
//      TODO: if identifier_location_index and identifier_location is provided,
//            then 'Parser_compile_variable_value_to_stack(parser, identifier_location, identifier_location_index);'
        parser_load_variable_value_to_stack(parser, superclass);

        Compiler_CompileInstruction_3Bytes(
            parser_get_current_bytecode(parser),
            OpCode_Call_Super_Method,
            method_location_index,
            argument_count,
            parser->token_previous.line_number
        );
    }
    else {
        parser_load_variable_value_to_stack(parser, superclass);
        // parser_load_variable_value_to_stack(parser, (Token) { 
        //     .kind = Token_Riba, 
        //     .start = "riba", 
        //     .length = 4
        // });
        Compiler_CompileInstruction_2Bytes(
            parser_get_current_bytecode(parser),
            OpCode_Get_Super,
            method_location_index,
            parser->token_previous.line_number
        );
    }
}

static Expression* parser_parse_expression(Parser* parser, OperatorPrecedence operator_precedence_previous) {
    parser_advance(parser);

    bool can_assign               = (operator_precedence_previous <= OperatorPrecedence_Assignment);
    Expression* expression        = NULL;
    int identifier_location       = -1;
    int identifier_location_index = -1;

//  Parsing Operands Section
    if (parser_is_literals(parser->token_previous)) {
        expression = parser_parse_literals(parser, can_assign); 
    }
    else if (parser_is_operator_unary(parser->token_previous)) {
        expression = parser_parse_operators_unary(parser); 
    }
    else if (parser->token_previous.kind == Token_Identifier)  {
        identifier_location_index = parser_find_identifier_location(parser, parser->token_previous, &identifier_location, NULL);

        if (parser->token_current.kind != Token_Equal) {
//          TODO: pass argument parser->function->depth
            if (parser_is_identifier_a_superclass(parser->function, parser->token_previous)) {
//              TODO: pass as arguments identifier_location_index & identifier_location
                parser_parse_superclass_method_call(parser);
            }
            else {
                Parser_compile_variable_value_to_stack(parser, identifier_location, identifier_location_index);
            }
        }
    } 
    else if (parser->token_previous.kind == Token_Keli) {
        if (parser->first_class_declaration == NULL) {
            parser_error(parser, &parser->token_previous, "Can't use 'keli' outside of a class.");
        } 
        else {
            identifier_location_index = parser_find_identifier_location(parser, parser->token_previous, &identifier_location, NULL);
            if (parser->token_current.kind != Token_Equal) 
                Parser_compile_variable_value_to_stack(parser, identifier_location, identifier_location_index); 
        }
    }
    else if (parser->token_previous.kind == Token_Riba) {
        parser_parse_superclass_method_call(parser);

        // if (parser->first_class_declaration == NULL) {
        //     parser_error(parser, &parser->token_previous, "Can't use 'riba' outside of a class.");
        // }
        // else if (!parser->first_class_declaration->has_superclass) {
        //     parser_error(parser, &parser->token_previous, "Can't use 'riba' in a class with no superclass.");
        // }
        //
        // parser_consume(parser, Token_Dot, "Expect '.' after 'riba'.");
        // parser_consume(parser, Token_Identifier, "Expect superclass method name.");
        //
        // int method_location = -1;
        // // TODO
        // // if location is on current Stack / Local -> print error 
        // // Identifier location index must be a method in a parent class
        // // TODO: rename to 'method_location_index'
        // int method_location_index = parser_find_identifier_location(parser, parser->token_previous, &method_location, NULL);
        // parser_load_variable_value_to_stack(parser, (Token) {
        //     .kind = Token_Keli,
        //     .start = "keli",
        //     .length = 4
        // });
        //
        // if (parser_match_then_advance(parser, Token_Left_Parenthesis)) {
        //     uint8_t argument_count = parser_parse_arguments(parser, Token_Left_Parenthesis);
        //     parser_load_variable_value_to_stack(parser, (Token) { 
        //         .kind = Token_Riba, 
        //         .start = "riba", 
        //         .length = 4
        //     });
        //     Compiler_CompileInstruction_2Bytes(
        //         parser_get_current_bytecode(parser),
        //         OpCode_Call_Super_Method,
        //         method_location_index,
        //         parser->token_previous.line_number
        //     );
        //     Compiler_CompileInstruction_1Byte(
        //         parser_get_current_bytecode(parser),
        //         argument_count,
        //         parser->token_previous.line_number
        //     );
        // }
        // else {
        //     parser_load_variable_value_to_stack(parser, (Token) { 
        //         .kind = Token_Riba, 
        //         .start = "riba", 
        //         .length = 4
        //     });
        //     Compiler_CompileInstruction_2Bytes(
        //         parser_get_current_bytecode(parser),
        //         OpCode_Get_Super,
        //         method_location_index,
        //         parser->token_previous.line_number
        //     );
        // }
    }

//  Parsing Operators Section
    OperatorMetadata operator_current = parser_get_operator_metadata(parser->token_current.kind);
    for (;;) {
        if (operator_current.is_left_associative)
        if (operator_current.precedence <= operator_precedence_previous) 
            break;

        if (operator_current.is_right_associative)
        if (operator_current.precedence < operator_precedence_previous)
            break;

        parser_advance(parser);

        if (parser_is_operator_arithmetic(parser->token_previous)) {
            expression = parser_parse_operators_arithmetic(parser, expression);
        } 
        else if (parser_is_operator_logical(parser->token_previous)) {
            expression = parser_parse_operators_logical(parser, expression);
        } 
        else if (parser_is_operator_relational(parser->token_previous)) {
            expression = parser_parse_operators_relational(parser, expression);
        } 
        else if (parser->token_previous.kind == Token_Left_Parenthesis) {
            expression = parser_parse_operator_function_call(parser, expression);
        } 
        else if (parser->token_previous.kind == Token_Left_Brace) {
            expression = parser_parse_operator_function_call(parser, expression);
        }
        else if (parser->token_previous.kind == Token_Dot) {
            expression = parser_parse_operator_object_getter_and_setter(parser, expression, can_assign);
        } 
        else if (parser->token_previous.kind == Token_Equal) {
            do {
                if (identifier_location_index == -1) break;
                if (!can_assign)                     break;

                parser_parse_operator_assignment(parser, identifier_location, identifier_location_index);
            } while (0);

//          If-Else Error Section
            if (identifier_location_index == -1) parser_error(parser, &parser->token_previous, "Undefined identifier.");    else 
            if (!can_assign)                     parser_error(parser, &parser->token_current, "Invalid assignment target.");
        }

        operator_current = parser_get_operator_metadata(parser->token_current.kind);
    }

    if (parser->token_current.kind == Token_Equal && operator_current.precedence < operator_precedence_previous) {
        parser_error(parser, &parser->token_current, "Invalid placement of Assignment");
    }

    return expression;
}

static Expression* parser_parse_literals(Parser* parser, bool can_assign) {
    if (parser->token_previous.kind == Token_Number) {
        double value = strtod(parser->token_previous.start, NULL);
        Value number = value_make_number(value);
        Expression e_number = expression_make_number(value);

        Compiler_CompileInstruction_Constant(
            parser_get_current_bytecode(parser), 
            number, 
            parser->token_previous.line_number
        );

        return expression_allocate(e_number);
    }

    if (parser->token_previous.kind == Token_Verdadi) {
        Expression e_true = expression_make_boolean(true);

        Compiler_CompileInstruction_1Byte(
            parser_get_current_bytecode(parser), 
            OpCode_Stack_Push_Literal_True, 
            parser->token_previous.line_number
        );

        return expression_allocate(e_true);
    }

    if (parser->token_previous.kind == Token_Falsu) {
        Expression e_false = expression_make_boolean(false);

        Compiler_CompileInstruction_1Byte(
            parser_get_current_bytecode(parser), 
            OpCode_Stack_Push_Literal_False, 
            parser->token_previous.line_number
        );

        return expression_allocate(e_false);
    }

    if (parser->token_previous.kind == Token_Nulo) {
        Expression nil = expression_make_nil();

        Compiler_CompileInstruction_1Byte(
            parser_get_current_bytecode(parser), 
            OpCode_Stack_Push_Literal_Nil, 
            parser->token_previous.line_number
        );

        return expression_allocate(nil);
    }

    if (parser->token_previous.kind == Token_String) {
        parser->token_previous.start  += 1;
        parser->token_previous.length -= 2;
        ObjectString* string = parser_intern_token(parser->token_previous, parser->object_head, parser->string_database);
        Value v_string = value_make_object(string);
        Memory_transaction_push(v_string);
        {
            Compiler_CompileInstruction_Constant(
                parser_get_current_bytecode(parser),
                v_string,
                parser->token_previous.line_number
            );
        }
        Memory_transaction_pop();

        Expression e_string = expression_make_string(string);
        return expression_allocate(e_string);
    }

    if (parser->token_previous.kind == Token_String_Interpolation) {
        for (;;)
        {
            if (parser->token_previous.kind != Token_String_Interpolation)
                break;

            ObjectString* string = parser_intern_token(parser->token_previous, parser->object_head, parser->string_database);
            Value v_string = value_make_object(string);
            Memory_transaction_push(v_string);
            {
                Compiler_CompileInstruction_Constant(
                    parser_get_current_bytecode(parser),
                    v_string,
                    parser->token_previous.line_number
                );
            }
            Memory_transaction_pop();

            parser->interpolation_count_value_pushed += 1;
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

        // TODO: use a Temporary Stack to process recursive calls
        // 
        // _parser_parse_unary_literals_and_identifier(parser, can_assign);

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

    return NULL;
}

static Expression* parser_parse_operators_unary(Parser* parser) {
    if (parser->token_previous.kind == Token_Minus) {
        Expression* expression = parser_parse_expression(parser, OperatorPrecedence_Negate);
        Expression negation = expression_make_negation(expression);

        Compiler_CompileInstruction_1Byte(
            parser_get_current_bytecode(parser), 
            OpCode_Negation, 
            parser->token_previous.line_number
        );

        return expression_allocate(negation);
    }

    if (parser->token_previous.kind == Token_Ka) {
        Expression* expression = parser_parse_expression(parser, OperatorPrecedence_Not);
        Expression not = expression_make_not(expression);

        Compiler_CompileInstruction_1Byte(
            parser_get_current_bytecode(parser), 
            OpCode_Not, 
            parser->token_previous.line_number
        );

        return expression_allocate(not);
    }

    if (parser->token_previous.kind == Token_Left_Parenthesis) {
        Expression* expression = parser_parse_expression(parser, OperatorPrecedence_Assignment);
        Expression grouping = expression_make_grouping(expression);

        parser_consume(parser, Token_Right_Parenthesis, "Expected ')' after expression.");
        return expression_allocate(grouping);
    }

    return NULL;
}

static bool parser_is_operator_arithmetic(Token token) {
    if (token.kind == Token_Plus)     return true;
    if (token.kind == Token_Minus)    return true;
    if (token.kind == Token_Asterisk) return true;
    if (token.kind == Token_Slash)    return true;
    if (token.kind == Token_Caret)    return true;

    return false;
}

static bool parser_is_operator_logical(Token token) {
    if (token.kind == Token_E)  return true;
    if (token.kind == Token_Ou) return true;
    if (token.kind == Token_Ka) return true;

    return false;
}

static bool parser_is_operator_relational(Token token) {
    if (token.kind == Token_Equal_Equal)    return true;
    if (token.kind == Token_Not_Equal)      return true;
    if (token.kind == Token_Less)           return true;
    if (token.kind == Token_Less_Equal)     return true;
    if (token.kind == Token_Greater)        return true;
    if (token.kind == Token_Greater_Equal)  return true;

    return false;
}

static bool parser_is_operator_assignment(Token token) {
    return (token.kind == Token_Equal);
}

static bool parser_is_operator_unary(Token token) {
    if (token.kind == Token_Minus)            return true;
    if (token.kind == Token_Ka)               return true;
    if (token.kind == Token_Left_Parenthesis) return true;

    return false;
}

static bool parser_is_literals(Token token) {
    if (token.kind == Token_Number)                 return true;
    if (token.kind == Token_Verdadi)                return true;
    if (token.kind == Token_Falsu)                  return true;
    if (token.kind == Token_Nulo)                   return true;
    if (token.kind == Token_String)                 return true;
    if (token.kind == Token_String_Interpolation)   return true;

    return false;
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
        Compiler_CompileInstruction_1Byte(parser_get_current_bytecode(parser), OpCode_Stack_Pop, parser->token_previous.line_number);

        Expression* rigth_operand = parser_parse_expression(parser, OperatorPrecedence_And);

        Compiler_PatchInstructionJump(parser_get_current_bytecode(parser), operand_index);

        Expression expression_and = expression_make_and(left_operand, rigth_operand);
        return expression_allocate(expression_and);
    } break;
    case Token_Ou: {
        int jump_if_false_operand_index = Compiler_CompileInstruction_Jump(parser_get_current_bytecode(parser), OpCode_Jump_If_False, parser->token_previous.line_number);
        int jump_operand_index = Compiler_CompileInstruction_Jump(parser_get_current_bytecode(parser), OpCode_Jump, parser->token_previous.line_number);

        Compiler_PatchInstructionJump(parser_get_current_bytecode(parser), jump_if_false_operand_index);
        Compiler_CompileInstruction_1Byte(parser_get_current_bytecode(parser), OpCode_Stack_Pop, parser->token_previous.line_number);

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
        Compiler_CompileInstruction_1Byte(parser_get_current_bytecode(parser), OpCode_Add, parser->token_previous.line_number);
        Expression addition = expression_make_addition(left_operand, right_operand);
        return expression_allocate(addition);
    } break;
    case Token_Minus: {
        Compiler_CompileInstruction_1Byte(parser_get_current_bytecode(parser), OpCode_Subtract, parser->token_previous.line_number);
        Expression subtraction = expression_make_subtraction(left_operand, right_operand);
        return expression_allocate(subtraction);
    } break;
    case Token_Asterisk: {
        Compiler_CompileInstruction_1Byte(parser_get_current_bytecode(parser), OpCode_Multiply, parser->token_previous.line_number);
        Expression multiplication = expression_make_multiplication(left_operand, right_operand);
        return expression_allocate(multiplication);
    } break;
    case Token_Slash: {
        Compiler_CompileInstruction_1Byte(parser_get_current_bytecode(parser), OpCode_Divide, parser->token_previous.line_number);
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

// TODO: static uint8_t parser_parse_arguments(Parser* parser, TokenKind kind)
static uint8_t parser_parse_arguments(Parser* parser, TokenKind token_kind) {
    uint8_t argument_count = 0;
    TokenKind closing_pair = (
        token_kind == Token_Left_Brace 
        ? Token_Right_Brace
        : Token_Right_Parenthesis
    );
    if (parser->token_current.kind != closing_pair) {
        do {
            Expression* expression = parser_parse_expression(parser, OperatorPrecedence_Assignment);
            if (argument_count == 255) {
                parser_error(parser, &parser->token_previous, "Can't have more than 255 arguments.");
            }
            argument_count += 1;
        } while (parser_match_then_advance(parser, Token_Comma));
    }

    parser_consume(parser, closing_pair,"Expect '}' after arguments");

    return argument_count;
}

static Expression* parser_parse_operator_function_call(Parser* parser, Expression* left_operand) {
    Token token_call = parser->token_previous;
    uint8_t argument_count = parser_parse_arguments(parser, parser->token_previous.kind);
    OpCode opcode = 0;
    if (token_call.kind == Token_Left_Brace) {
        opcode = OpCode_Call_Class;
    }
    else {
        opcode = OpCode_Call_Function;
    }
    Compiler_CompileInstruction_2Bytes(
        parser_get_current_bytecode(parser),
        opcode,         // OpCode
        argument_count, // Operand
        parser->token_previous.line_number
    );
    return NULL;
}

static Expression* parser_parse_operator_object_getter_and_setter(Parser* parser, Expression* left_expression, bool can_assign) {
    parser_consume(parser, Token_Identifier, "Expected property name after '.'.");

    // Register Identifier into Bytecode Values Array
    //
    String source_string = string_make(parser->token_previous.start, parser->token_previous.length);
    uint32_t source_hash = string_hash(source_string);
    ObjectString* identifier_string = ObjectString_Allocate(
        .task = (
            AllocateTask_Check_If_Interned  |
            AllocateTask_Copy_String        |
            AllocateTask_Initialize         |
            AllocateTask_Intern
        ),
        .string = source_string,
        .hash   = source_hash,
        .first  = parser->object_head,
        .table  = parser->string_database
    );

    Value value_string = value_make_object_string(identifier_string);
    int property_name_index = -1;
    Memory_transaction_push(value_string);
    {
        property_name_index = Compiler_CompileValue(
            parser_get_current_bytecode(parser), 
            value_string
        );
    }
    Memory_transaction_pop();

    if (can_assign && parser_match_then_advance(parser, Token_Equal)) {
        parser_parse_expression(parser, OperatorPrecedence_Assignment);
        Compiler_CompileInstruction_2Bytes(
            parser_get_current_bytecode(parser),
            OpCode_Object_Set_Property,             // OpCode
            property_name_index,                    // Operand
            parser->token_previous.line_number
        );
    }
    else if (parser_match_then_advance(parser, Token_Left_Parenthesis)) {
        uint8_t argument_count = parser_parse_arguments(parser, Token_Left_Parenthesis);
        Compiler_CompileInstruction_3Bytes(
            parser_get_current_bytecode(parser),
            OpCode_Call_Method,  // OpCode
            property_name_index, // Operand 1
            argument_count,      // Operand 2
            parser->token_previous.line_number
        );
    } 
    else {
        Compiler_CompileInstruction_2Bytes(
            parser_get_current_bytecode(parser),
            OpCode_Object_Get_Property,             // OpCode
            property_name_index,                    // Operand
            parser->token_previous.line_number
        );
    }

    return NULL;
}

static bool parser_check_locals_duplicates(Parser* parser, Token* identifier) {
    StackLocal* locals = &parser->function->locals;
    for (int i = locals->top - 1; i >= 0; i--) {
        Local* local = &locals->items[i];
        if (
            local->scope_depth < parser->function->depth &&
            local->scope_depth != -1
        ) {
            break;
        }

        if (token_is_identifier_equal(identifier, &local->token))
            return true;
    }

    return false;
}

static void parser_begin_scope(Parser* parser) {
    // parser->scope.depth += 1;
    parser->function->depth += 1;
}

static void parser_end_scope(Parser* parser) {
    parser->function->depth -= 1;

    StackLocal* locals = &parser->function->locals;
    for (;;) {
        if (StackLocal_is_empty(locals)) break;
        if (StackLocal_peek(locals, 0)->scope_depth <= parser->function->depth) break;

        if (StackLocal_peek(locals, 0)->action == LocalAction_Move_Heap) {
            Compiler_CompileInstruction_1Byte(
                parser_get_current_bytecode(parser),
                OpCode_Stack_Move_Value_To_Heap,              // TODO: rename to 'OpCode_Stack_Move_Value_To_Heap'
                parser->token_previous.line_number
            );
        } 
        else {
            Compiler_CompileInstruction_1Byte(
                parser_get_current_bytecode(parser),
                OpCode_Stack_Pop,
                parser->token_previous.line_number
            );
        }

        StackLocal_pop(locals);
    }
}

static void parser_init_function(Parser* parser, Function* function, FunctionKind function_kind, Token class_name) {
    ObjectFunction* object_fn = ObjectFunction_allocate(parser->object_head);
    if (function_kind != FunctionKind_Script) 
        object_fn->name = parser_intern_token(parser->token_previous, parser->object_head, parser->string_database);

    Local local       = {0};
    local.token.start = "";
    local.scope_depth = 0;
    local.action      = LocalAction_Default;
//  TODO: test 'Keli' keyword for a Script function
    if (function_kind != FunctionKind_Function) {
        local.token.kind   = Token_Keli;
        local.token.start  = "keli";
        local.token.length = 4;
    }

    function->function_kind = function_kind;
    function->object        = NULL;
    function->object        = object_fn;
    function->depth         = 0;
    function->class_name    = (Token) {0};
    if (class_name.kind != Token_Nil) 
        function->class_name = class_name;

    StackLocal_init(&function->locals);
    StackLocal_push(&function->locals, local);
    ArrayLocalMetadata_init(&function->variable_dependencies, 0);
    LinkedList_push(parser->function, function);
    DynamicArray_push(&parser->debugger_functions, *function);
}

static ObjectFunction* parser_end_function(Parser* parser, Function* function) {
    ObjectFunction* object_fn = parser->function->object;

    parser_compile_return(parser);

    ///NOTE: I dont need to free(popped_function), because its a 
    //       stack value and it will be discaded by the caller function when returned.
    Function* popped_function = NULL;
    LinkedList_pop(parser->function, popped_function);
    DynamicArray_pop(&parser->debugger_functions, popped_function);

#ifdef DEBUG_COMPILER_BYTECODE
    if (!parser_has_error) {
        Bytecode_disassemble(
            &function->bytecode,
            function->name != NULL
            ? function->name->characters
            : "<script>"
        );
    }
#endif

    return object_fn;
}

// NOTE: If a closure (child function) access a variable from a parent, then it will
//       traverse all the parent hierarchy to find the local. Once its found,
//       the variable will be copyed in all the function the traversal had to
//       to go trough to find the variabe. 
//
int parser_resolve_variable_dependencies(Function* function, Token* name, Local** ret_local) {
    StackFunction stack_function = { 0 };
    int local_found_idx = -1;

    StackFunction_push(&stack_function, function);

    Function* current_function = function->next;
    for (;;) {
        if (current_function == NULL) break;

        local_found_idx = StackLocal_get_local_index_by_token(&current_function->locals, name, NULL);
        if (local_found_idx != -1) break; // if found token, break ot of thhe loop

        StackFunction_push(&stack_function, current_function);

        current_function = current_function->next;
    }

    // if (local_found_idx == -1) {
    //     StackTemp_clear();
    //     return -1;
    // }
    //
    if (local_found_idx == -1) return -1;

    current_function->locals.items[local_found_idx].action = LocalAction_Move_Heap;
    *ret_local = &current_function->locals.items[local_found_idx];

    int parent_local_idx = local_found_idx;
    bool is_top_item = true;
    for (;;) {
        if (StackFunction_is_empty(&stack_function)) break;

        current_function = StackFunction_peek(&stack_function, 0);
        parent_local_idx = ArrayLocalMetadata_add(
            &current_function->variable_dependencies,
            (uint8_t)parent_local_idx,
            (is_top_item 
                ? LocalLocation_In_Parent_Stack 
                : LocalLocation_In_Parent_Heap_Values
            ),
            &current_function->object->variable_dependencies_count
        );

        if (is_top_item) is_top_item = false;
        StackFunction_pop(&stack_function);
    }

    return parent_local_idx;
}

void Parser_free(Parser* parser) {
// NOTE: Do not free parseer->objects, because Virtual Machine will need it
//
// TODO: Missing Implementation
//
}