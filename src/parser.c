#include "kriolu.h"

// TODO:
// [x] Encapsulate expression allocation function in the expression.c file
// [] Refactor parser interface public and private
// [x] Change ExpresisonAst to Expression
// [x] Add statement in StatementAstArray
// [] Deallocate statement
// [] Emit bytecode instructions
// [] Write test

Parser *parser_global;
Lexer parser_lexer_global;
int debug_line_number = 1;

void parser_init(Parser *parser, Lexer *lexer, bool set_global)
{
    parser->current = (Token){0};  // token_error
    parser->previous = (Token){0}; // token_error
    parser->lexer = lexer;
    if (parser->lexer == NULL)
    {
        parser->lexer = &parser_lexer_global;
        // todo: review this later. parser doen't have source code to init lexer
        // lexer_init();
    }
    parser->panic_mode = false;
    parser->had_error = false;

    if (set_global)
    {
        parser_global = parser;
    }
}

StatementArray *parser_parse(Parser *parser)
{
    StatementArray *statements = statement_array_allocate();
    parser_advance(parser);

    for (;;)
    {
        if (parser->current.kind == TOKEN_EOF)
            break;

        Statement statement = parser_statement(parser);
        statement_array_insert(statements, statement);

        // #ifdef DEBUG_LOG_PARSER
        //         parser_expression_print_ast(statement.expression);
        //         printf("\n");
        //         parser_expression_free_ast(statement.expression);
        // #endif
    }

    return statements;
}

void parser_advance(Parser *parser)
{
    parser->previous = parser->current;

    for (;;)
    {
        parser->current = lexer_scan(parser->lexer);

        // todo: skip comment token
        if (parser->current.kind != TOKEN_ERROR)
            break;

        parser_error(parser, &parser->current, "Unexpected character!");
    }
}

bool parser_match_then_advance(Parser *parser, TokenKind kind)
{

    if (parser->current.kind == kind)
    {
        parser_advance(parser);
        return true;
    }

    return false;
}

void parser_consume(Parser *parser, TokenKind kind, const char *error_message)
{
    if (parser->current.kind == kind)
    {
        parser_advance(parser);
        return;
    }

    parser_error(parser, &parser->previous, error_message);
}

void parser_synchronize(Parser *parser)
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

void parser_error(Parser *parser, Token *token, const char *message)
{
    if (parser->panic_mode)
        return;

    parser->panic_mode = true;
    parser->had_error = true;

    fprintf(stderr, "[line %d] Error", token->line_number);
    if (token->kind == TOKEN_EOF)
    {
        fprintf(stderr, " at end");
    }
    else
    {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }
    fprintf(stderr, " : '%s'\n", message);
}

Statement parser_statement(Parser *parser)
{
    // DECLARATIONS: introduces new identifiers
    //   klassi, mimoria, funson
    // INSTRUCTIONS: tells the computer to perform an action
    //   imprimi, di, si, divolvi, ripiti, block
    // EXPRESSIONS: a calculation that produces a result
    //   +, -, /, *, call '(', assignment '=', objectGet '.'
    //   variableGet, literals

    Statement expression_statement = parser_expression_statement(parser);

    if (parser->panic_mode)
        parser_synchronize(parser);

    return expression_statement;
}

Statement parser_expression_statement(Parser *parser)
{

    Expression *expression = parser_expression(parser, OPERATION_ASSIGNMENT);
    parser_consume(parser, TOKEN_SEMICOLON, "Expect ';' after expression.");

    // todo: deallocate statement
    // Statement *statement = calloc(1, sizeof(Statement));
    // assert(statement);
    // statement->kind = StatementKind_Expression;
    // statement->expression = expression;

    Statement statement = (Statement){
        .kind = StatementKind_Expression,
        .expression = expression};

    return statement;
}

static OrderOfOperation parser_operator_precedence(TokenKind kind)
{
    switch (kind)
    {
    default:
        return 0;
    case TOKEN_EQUAL:
        return OPERATION_ASSIGNMENT;
    case TOKEN_OU:
        return OPERATION_OR;
    case TOKEN_E:
        return OPERATION_AND;
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
    case TOKEN_CARET:
        return OPERATION_EXPONENTIATION;
    case TOKEN_KA:
        return OPERATION_NOT;
    case TOKEN_DOT:
    case TOKEN_LEFT_PARENTHESIS:
        return OPERATION_GROUPING_CALL_AND_GET;
    }
}

Expression *parser_expression(Parser *parser, OrderOfOperation operator_precedence_previous)
{
    parser_advance(parser);

    Expression *left_operand = parser_unary_and_literals(parser);
    Expression *expression = left_operand;

    TokenKind operator_kind_current = parser->current.kind;
    OrderOfOperation operator_precedence_current = parser_operator_precedence(operator_kind_current);

    while (operator_precedence_current > operator_precedence_previous)
    {
        parser_advance(parser);
        expression = parser_binary(parser, left_operand);

        operator_precedence_current = parser_operator_precedence(parser->current.kind);
    }

    return expression;
}

Expression *parser_unary_and_literals(Parser *parser)
{
    // todo: emit bytecode instructions
    // todo: change name ret to expression
    Expression *return_value = NULL;

    if (parser->previous.kind == TOKEN_NUMBER)
    {
        double value = strtod(parser->previous.start, NULL);
        return_value = expression_allocate_number(value);
    }
    else if (parser->previous.kind == TOKEN_MINUS)
    {
        Expression *expression = parser_expression(parser, OPERATION_NEGATE);
        return_value = expression_allocate_negation(expression);
    }
    else if (parser->previous.kind == TOKEN_LEFT_PARENTHESIS)
    {
        Expression *expression = parser_expression(parser, OPERATION_ASSIGNMENT);
        return_value = expression_allocate_grouping(expression);

        parser_consume(parser, TOKEN_RIGHT_PARENTHESIS, "Expected ')' after expression.");
    }

    return return_value;
}

Expression *parser_binary(Parser *parser, Expression *left_operand)
{
    TokenKind operator_kind_previous = parser->previous.kind;
    OrderOfOperation operator_precedence_previous = parser_operator_precedence(operator_kind_previous);

    Expression *right_operand = parser_expression(parser, operator_precedence_previous);

    if (operator_kind_previous == TOKEN_PLUS)
    {
        return expression_allocate_binary(ExpressionKind_Addition, left_operand, right_operand);
    }
    else if (operator_kind_previous == TOKEN_MINUS)
    {
        return expression_allocate_binary(ExpressionKind_Subtraction, left_operand, right_operand);
    }
    else if (operator_kind_previous == TOKEN_ASTERISK)
    {
        return expression_allocate_binary(ExpressionKind_Multiplication, left_operand, right_operand);
    }
    else if (operator_kind_previous == TOKEN_SLASH)
    {
        return expression_allocate_binary(ExpressionKind_Division, left_operand, right_operand);
    }
    else if (operator_kind_previous == TOKEN_CARET)
    {
        return expression_allocate_binary(ExpressionKind_Exponentiation, left_operand, right_operand);
    }

    return NULL;
}

Expression *parser_expression_allocate_ast(Expression ast)
{
    Expression *ast_expression = (Expression *)malloc(sizeof(ast));
    if (ast_expression == NULL)
    {
        exit(EXIT_FAILURE);
    }

    *ast_expression = ast;

    return ast_expression;
}

// todo: remove
void parser_expression_free_ast(Expression *expression)
{

    if (expression->kind == ExpressionKind_Number)
    {
        ExpressionNumber number = expression->number;
    }
    else if (expression->kind == ExpressionKind_Negation)
    {
        ExpressionNegation negation = expression->negation;
        parser_expression_free_ast(negation.operand);
    }
    else if (expression->kind == ExpressionKind_Grouping)
    {
        ExpressionGrouping grouping = expression->grouping;
        parser_expression_free_ast(grouping.expression);
    }
    else if (expression->kind == ExpressionKind_Addition)
    {
        ExpressionAddition addition = expression->addition;
        parser_expression_free_ast(addition.left_operand);
        parser_expression_free_ast(addition.right_operand);
    }
    else if (expression->kind == ExpressionKind_Subtraction)
    {
        ExpressionSubtraction subtraction = expression->subtraction;
        parser_expression_free_ast(subtraction.left_operand);
        parser_expression_free_ast(subtraction.right_operand);
    }
    else if (expression->kind == ExpressionKind_Multiplication)
    {
        ExpressionMultiplication multiplication = expression->multiplication;
        parser_expression_free_ast(multiplication.left_operand);
        parser_expression_free_ast(multiplication.right_operand);
    }
    else if (expression->kind == ExpressionKind_Division)
    {
        ExpressionDivision division = expression->division;
        parser_expression_free_ast(division.left_operand);
        parser_expression_free_ast(division.right_operand);
    }
    else if (expression->kind == ExpressionKind_Exponentiation)
    {
        ExpressionExponentiation exponentiation = expression->exponentiation;
        parser_expression_free_ast(exponentiation.left_operand);
        parser_expression_free_ast(exponentiation.right_operand);
    }
    else
    {
        printf("warning: Unrecognized node type!");
    }

    free(expression);
}

void parser_expression_print_ast(Expression *expression)
{

    if (expression->kind == ExpressionKind_Number)
    {
        ExpressionNumber number = expression->number;
        printf("%.1f", number.value);
    }
    else if (expression->kind == ExpressionKind_Negation)
    {
        ExpressionNegation negation = expression->negation;
        printf("(");
        printf("-");
        parser_expression_print_ast(negation.operand);
        printf(")");
    }
    else if (expression->kind == ExpressionKind_Grouping)
    {
        ExpressionGrouping grouping = expression->grouping;
        parser_expression_print_ast(grouping.expression);
    }
    else if (expression->kind == ExpressionKind_Addition)
    {
        ExpressionAddition addition = expression->addition;
        printf("(");
        parser_expression_print_ast(addition.left_operand);
        printf(" + ");
        parser_expression_print_ast(addition.right_operand);
        printf(")");
    }
    else if (expression->kind == ExpressionKind_Subtraction)
    {
        ExpressionSubtraction subtraction = expression->subtraction;
        printf("(");
        parser_expression_print_ast(subtraction.left_operand);
        printf(" - ");
        parser_expression_print_ast(subtraction.right_operand);
        printf(")");
    }
    else if (expression->kind == ExpressionKind_Multiplication)
    {
        ExpressionMultiplication multiplication = expression->multiplication;
        printf("(");
        parser_expression_print_ast(multiplication.left_operand);
        printf(" * ");
        parser_expression_print_ast(multiplication.right_operand);
        printf(")");
    }
    else if (expression->kind == ExpressionKind_Division)
    {
        ExpressionDivision division = expression->division;
        printf("(");
        parser_expression_print_ast(division.left_operand);
        printf(" / ");
        parser_expression_print_ast(division.right_operand);
        printf(")");
    }
    else if (expression->kind == ExpressionKind_Exponentiation)
    {
        ExpressionExponentiation exponentiation = expression->exponentiation;
        printf("(");
        parser_expression_print_ast(exponentiation.left_operand);
        printf(" ^ ");
        parser_expression_print_ast(exponentiation.right_operand);
        printf(")");
    }
    else
    {
        printf("Error: node not supported!");
    }
}
