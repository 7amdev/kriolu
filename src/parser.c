#include "kriolu.h"

// todo: emit bytecode instructions
// todo: call error function
// todo: refactor parser interface public and private
// todo: parser option to output postfix notation or parenthesis
// todo: write test
// todo: macro flag to print debug info: postfix and parenthesis notation

parser_t *parser_global;
int debug_line_number = 1;

void parser_init(parser_t *parser, lexer_t *lexer, bool set_global)
{
    parser->current = (token_t){0};  // token_error
    parser->previous = (token_t){0}; // token_error
    parser->lexer = lexer;
    parser->panic_mode = false;
    parser->had_error = false;

    if (set_global)
    {
        parser_global = parser;
    }
}

void parser_parse(parser_t *parser)
{
    parser_advance(parser);

    for (;;)
    {
        if (parser->current.kind == TOKEN_EOF)
            break;

        parser_declaration(parser);
    }
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

void parser_declaration(parser_t *parser)
{
    // for now
    parser_expression_statement(parser);

    // todo: test parser_synchronize
    if (parser->panic_mode)
        parser_synchronize(parser);
}

void parser_expression_statement(parser_t *parser)
{
    AstExpression *expression;

    expression = parser_expression(parser, OPERATION_ASSIGNMENT);
    parser_consume(parser, TOKEN_SEMICOLON, "Expect ';' after expression.");

#ifdef DEBUG_LOG_PARSER
    parser_ast_expression_print(expression);
    printf("\n");
    parser_ast_expression_free(expression);
#endif
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

AstExpression *parser_expression(parser_t *parser, OrderOfOperation operator_precedence_previous)
{
    parser_advance(parser);
    AstExpression *left_operand = parser_unary_and_literals(parser);

    token_kind_t operator_kind_current = parser->current.kind;
    OrderOfOperation operator_precedence_current = parser_operator_precedence(operator_kind_current);

    while (operator_precedence_current > operator_precedence_previous)
    {
        parser_advance(parser);
        left_operand = parser_binary(parser, left_operand);

        operator_precedence_current = parser_operator_precedence(parser->current.kind);
    }

    return left_operand;
}

AstExpression *parser_unary_and_literals(parser_t *parser)
{
    // todo: emit bytecode instructions
    AstExpression *ret = NULL;

    if (parser->previous.kind == TOKEN_NUMBER)
    {
        double value = strtod(parser->previous.start, NULL);
        AstNodeNumber number = (AstNodeNumber){
            .value = value};

        ret = parser_ast_expression_allocate((AstExpression){
            .kind = AST_NODE_NUMBER,
            .number = number});
    }
    else if (parser->previous.kind == TOKEN_MINUS)
    {
        AstExpression *result = parser_expression(parser, OPERATION_NEGATE);
        AstNodeNegation negation = (AstNodeNegation){
            .operand = result};

        ret = parser_ast_expression_allocate((AstExpression){
            .kind = AST_NODE_NEGATION,
            .negation = negation});
    }
    else if (parser->previous.kind == TOKEN_LEFT_PARENTHESIS)
    {
        AstExpression *expression = parser_expression(parser, OPERATION_ASSIGNMENT);
        AstNodeGrouping grouping = (AstNodeGrouping){
            .operand = expression};

        ret = parser_ast_expression_allocate((AstExpression){
            .kind = AST_NODE_GROUPING,
            .grouping = grouping});

        parser_consume(parser, TOKEN_RIGHT_PARENTHESIS, "Expected ')' after expression.");
    }

    return ret;
}

AstExpression *parser_binary(parser_t *parser, AstExpression *left_operand)
{
    AstExpression *ret = NULL;

    token_kind_t operator_kind_previous = parser->previous.kind;
    OrderOfOperation operator_precedence_previous = parser_operator_precedence(operator_kind_previous);

    AstExpression *right_operand = parser_expression(parser, operator_precedence_previous);

    if (operator_kind_previous == TOKEN_PLUS)
    {
        AstNodeAddition addition = (AstNodeAddition){
            .left_operand = left_operand,
            .right_operand = right_operand};

        AstExpression addition_expression = (AstExpression){
            .kind = AST_NODE_ADDITION,
            .addition = addition};

        ret = parser_ast_expression_allocate(addition_expression);
    }
    else if (operator_kind_previous == TOKEN_MINUS)
    {
        AstNodeSubtraction subtraction = (AstNodeSubtraction){
            .left_operand = left_operand,
            .right_operand = right_operand};

        AstExpression subtraction_expression = (AstExpression){
            .kind = AST_NODE_SUBTRACTION,
            .subtraction = subtraction};

        ret = parser_ast_expression_allocate(subtraction_expression);
    }
    else if (operator_kind_previous == TOKEN_ASTERISK)
    {
        AstNodeMultiplication multiplication = (AstNodeMultiplication){
            .left_operand = left_operand,
            .right_operand = right_operand};

        AstExpression multiplication_expression = (AstExpression){
            .kind = AST_NODE_MULTIPLICATION,
            .multiplication = multiplication};

        ret = parser_ast_expression_allocate(multiplication_expression);
    }
    else if (operator_kind_previous == TOKEN_SLASH)
    {
        AstNodeDivision division = (AstNodeDivision){
            .left_operand = left_operand,
            .right_operand = right_operand};

        AstExpression division_expression = (AstExpression){
            .kind = AST_NODE_DIVISION,
            .division = division};

        ret = parser_ast_expression_allocate(division_expression);
    }
    else if (operator_kind_previous == TOKEN_CARET)
    {
        AstNodeExponentiation exponential = (AstNodeExponentiation){
            .left_operand = left_operand,
            .right_operand = right_operand};

        AstExpression exponential_expression = (AstExpression){
            .kind = AST_NODE_EXPONENTIATION,
            .exponentiation = exponential};

        ret = parser_ast_expression_allocate(exponential_expression);
    }

    return ret;
}

AstExpression *parser_ast_expression_allocate(AstExpression ast)
{
    AstExpression *ast_expression = malloc(sizeof(ast));
    if (ast_expression == NULL)
    {
        exit(EXIT_FAILURE);
    }

    *ast_expression = ast;

    return ast_expression;
}

void parser_ast_expression_free(AstExpression *ast_node)
{

    if (ast_node->kind == AST_NODE_NUMBER)
    {
        AstNodeNumber number = ast_node->number;
    }
    else if (ast_node->kind == AST_NODE_NEGATION)
    {
        AstNodeNegation negation = ast_node->negation;
        parser_ast_expression_free(negation.operand);
    }
    else if (ast_node->kind == AST_NODE_GROUPING)
    {
        AstNodeGrouping grouping = ast_node->grouping;
        parser_ast_expression_free(grouping.operand);
    }
    else if (ast_node->kind == AST_NODE_ADDITION)
    {
        AstNodeAddition addition = ast_node->addition;
        parser_ast_expression_free(addition.left_operand);
        parser_ast_expression_free(addition.right_operand);
    }
    else if (ast_node->kind == AST_NODE_SUBTRACTION)
    {
        AstNodeSubtraction subtraction = ast_node->subtraction;
        parser_ast_expression_free(subtraction.left_operand);
        parser_ast_expression_free(subtraction.right_operand);
    }
    else if (ast_node->kind == AST_NODE_MULTIPLICATION)
    {
        AstNodeMultiplication multiplication = ast_node->multiplication;
        parser_ast_expression_free(multiplication.left_operand);
        parser_ast_expression_free(multiplication.right_operand);
    }
    else if (ast_node->kind == AST_NODE_DIVISION)
    {
        AstNodeDivision division = ast_node->division;
        parser_ast_expression_free(division.left_operand);
        parser_ast_expression_free(division.right_operand);
    }
    else if (ast_node->kind == AST_NODE_EXPONENTIATION)
    {
        AstNodeExponentiation exponentiation = ast_node->exponentiation;
        parser_ast_expression_free(exponentiation.left_operand);
        parser_ast_expression_free(exponentiation.right_operand);
    }
    else
    {
        printf("warning: Unrecognized node type!");
    }

    free(ast_node);
}

void parser_ast_expression_print(AstExpression *ast_node)
{

    if (ast_node->kind == AST_NODE_NUMBER)
    {
        AstNodeNumber number = ast_node->number;
        printf("%.1f", number.value);
    }
    else if (ast_node->kind == AST_NODE_NEGATION)
    {
        AstNodeNegation negation = ast_node->negation;
        printf("(");
        printf("-");
        parser_ast_expression_print(negation.operand);
        printf(")");
    }
    else if (ast_node->kind == AST_NODE_GROUPING)
    {
        AstNodeGrouping grouping = ast_node->grouping;
        parser_ast_expression_print(grouping.operand);
    }
    else if (ast_node->kind == AST_NODE_ADDITION)
    {
        AstNodeAddition addition = ast_node->addition;
        printf("(");
        parser_ast_expression_print(addition.left_operand);
        printf(" + ");
        parser_ast_expression_print(addition.right_operand);
        printf(")");
    }
    else if (ast_node->kind == AST_NODE_SUBTRACTION)
    {
        AstNodeSubtraction subtraction = ast_node->subtraction;
        printf("(");
        parser_ast_expression_print(subtraction.left_operand);
        printf(" - ");
        parser_ast_expression_print(subtraction.right_operand);
        printf(")");
    }
    else if (ast_node->kind == AST_NODE_MULTIPLICATION)
    {
        AstNodeMultiplication multiplication = ast_node->multiplication;
        printf("(");
        parser_ast_expression_print(multiplication.left_operand);
        printf(" * ");
        parser_ast_expression_print(multiplication.right_operand);
        printf(")");
    }
    else if (ast_node->kind == AST_NODE_DIVISION)
    {
        AstNodeDivision division = ast_node->division;
        printf("(");
        parser_ast_expression_print(division.left_operand);
        printf(" / ");
        parser_ast_expression_print(division.right_operand);
        printf(")");
    }
    else if (ast_node->kind == AST_NODE_EXPONENTIATION)
    {
        AstNodeExponentiation exponentiation = ast_node->exponentiation;
        printf("(");
        parser_ast_expression_print(exponentiation.left_operand);
        printf(" ^ ");
        parser_ast_expression_print(exponentiation.right_operand);
        printf(")");
    }
    else
    {
        printf("Error: node not supported!");
    }
}
