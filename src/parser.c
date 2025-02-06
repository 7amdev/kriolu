#include "kriolu.h"

// TODO:
// [] Add statement in StatementAstArray
// [] Deallocate statement
// [] Emit bytecode instructions
// [] Refactor parser interface public and private
// [] Write test

parser_t *parser_global;
lexer_t parser_lexer_global;
int debug_line_number = 1;

void parser_init(parser_t *parser, lexer_t *lexer, bool set_global)
{
    parser->current = (token_t){0};  // token_error
    parser->previous = (token_t){0}; // token_error
    if (lexer == NULL)
    {
        parser->lexer = &parser_lexer_global;
    }
    else
    {

        parser->lexer = lexer;
    }
    parser->panic_mode = false;
    parser->had_error = false;

    if (set_global)
    {
        parser_global = parser;
    }
}

StatementArray *parser_parse(parser_t *parser)
{
    StatementArray *statements = statement_array_allocate();
    parser_advance(parser);

    for (;;)
    {
        if (parser->current.kind == TOKEN_EOF)
            break;

        // todo: Add statement in StatementAstArray
        //       A program is a sequence of Statements
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

void parser_advance(parser_t *parser)
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

bool parser_match_then_advance(parser_t *parser, token_kind_t kind)
{

    if (parser->current.kind == kind)
    {
        parser_advance(parser);
        return true;
    }

    return false;
}

void parser_consume(parser_t *parser, token_kind_t kind, const char *error_message)
{
    if (parser->current.kind == kind)
    {
        parser_advance(parser);
        return;
    }

    parser_error(parser, &parser->previous, error_message);
}

void parser_synchronize(parser_t *parser)
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

void parser_error(parser_t *parser, token_t *token, const char *message)
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

Statement parser_statement(parser_t *parser)
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

Statement parser_expression_statement(parser_t *parser)
{

    ExpressionAst *expression = parser_expression(parser, OPERATION_ASSIGNMENT);
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

static OrderOfOperation parser_operator_precedence(token_kind_t kind)
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

ExpressionAst *parser_expression(parser_t *parser, OrderOfOperation operator_precedence_previous)
{
    parser_advance(parser);

    ExpressionAst *left_operand = parser_unary_and_literals(parser);
    ExpressionAst *expression = left_operand;

    token_kind_t operator_kind_current = parser->current.kind;
    OrderOfOperation operator_precedence_current = parser_operator_precedence(operator_kind_current);

    while (operator_precedence_current > operator_precedence_previous)
    {
        parser_advance(parser);
        // left_operand = parser_binary(parser, left_operand);
        expression = parser_binary(parser, left_operand);

        operator_precedence_current = parser_operator_precedence(parser->current.kind);
    }

    // return left_operand;
    return expression;
}

ExpressionAst *parser_unary_and_literals(parser_t *parser)
{
    // todo: emit bytecode instructions
    // todo: change name ret to expression
    ExpressionAst *ret = NULL;

    if (parser->previous.kind == TOKEN_NUMBER)
    {
        double value = strtod(parser->previous.start, NULL);
        ExpressionNumber number = (ExpressionNumber){
            .value = value};

        ret = parser_expression_allocate_ast((ExpressionAst){
            .kind = ExpressionKind_Number,
            .number = number});
    }
    else if (parser->previous.kind == TOKEN_MINUS)
    {
        ExpressionAst *result = parser_expression(parser, OPERATION_NEGATE);
        ExpressionNegation negation = (ExpressionNegation){
            .operand = result};

        ret = parser_expression_allocate_ast((ExpressionAst){
            .kind = ExpressionKind_Negation,
            .negation = negation});
    }
    else if (parser->previous.kind == TOKEN_LEFT_PARENTHESIS)
    {
        ExpressionAst *expression = parser_expression(parser, OPERATION_ASSIGNMENT);
        ExpressionGrouping grouping = (ExpressionGrouping){
            .operand = expression};

        ret = parser_expression_allocate_ast((ExpressionAst){
            .kind = ExpressionKind_Grouping,
            .grouping = grouping});

        parser_consume(parser, TOKEN_RIGHT_PARENTHESIS, "Expected ')' after expression.");
    }

    return ret;
}

ExpressionAst *parser_binary(parser_t *parser, ExpressionAst *left_operand)
{
    ExpressionAst *ret = NULL;

    token_kind_t operator_kind_previous = parser->previous.kind;
    OrderOfOperation operator_precedence_previous = parser_operator_precedence(operator_kind_previous);

    ExpressionAst *right_operand = parser_expression(parser, operator_precedence_previous);

    if (operator_kind_previous == TOKEN_PLUS)
    {
        ExpressionAddition addition = (ExpressionAddition){
            .left_operand = left_operand,
            .right_operand = right_operand};

        ExpressionAst addition_expression = (ExpressionAst){
            .kind = ExpressionKind_Addition,
            .addition = addition};

        ret = parser_expression_allocate_ast(addition_expression);
    }
    else if (operator_kind_previous == TOKEN_MINUS)
    {
        ExpressionSubtraction subtraction = (ExpressionSubtraction){
            .left_operand = left_operand,
            .right_operand = right_operand};

        ExpressionAst subtraction_expression = (ExpressionAst){
            .kind = ExpressionKind_Subtraction,
            .subtraction = subtraction};

        ret = parser_expression_allocate_ast(subtraction_expression);
    }
    else if (operator_kind_previous == TOKEN_ASTERISK)
    {
        ExpressionMultiplication multiplication = (ExpressionMultiplication){
            .left_operand = left_operand,
            .right_operand = right_operand};

        ExpressionAst multiplication_expression = (ExpressionAst){
            .kind = ExpressionKind_Multiplication,
            .multiplication = multiplication};

        ret = parser_expression_allocate_ast(multiplication_expression);
    }
    else if (operator_kind_previous == TOKEN_SLASH)
    {
        ExpressionDivision division = (ExpressionDivision){
            .left_operand = left_operand,
            .right_operand = right_operand};

        ExpressionAst division_expression = (ExpressionAst){
            .kind = ExpressionKind_Division,
            .division = division};

        ret = parser_expression_allocate_ast(division_expression);
    }
    else if (operator_kind_previous == TOKEN_CARET)
    {
        ExpressionExponentiation exponential = (ExpressionExponentiation){
            .left_operand = left_operand,
            .right_operand = right_operand};

        ExpressionAst exponential_expression = (ExpressionAst){
            .kind = ExpressionKind_Exponentiation,
            .exponentiation = exponential};

        ret = parser_expression_allocate_ast(exponential_expression);
    }

    return ret;
}

ExpressionAst *parser_expression_allocate_ast(ExpressionAst ast)
{
    ExpressionAst *ast_expression = (ExpressionAst *)malloc(sizeof(ast));
    if (ast_expression == NULL)
    {
        exit(EXIT_FAILURE);
    }

    *ast_expression = ast;

    return ast_expression;
}

void parser_expression_free_ast(ExpressionAst *expression)
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
        parser_expression_free_ast(grouping.operand);
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

void parser_expression_print_ast(ExpressionAst *expression)
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
        parser_expression_print_ast(grouping.operand);
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
