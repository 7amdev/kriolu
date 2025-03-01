#include "kriolu.h"

Expression *expression_allocate(Expression expr)
{
    Expression *expression = (Expression *)calloc(1, sizeof(Expression));
    assert(expression);

    // init
    *expression = expr;

    return expression;
}

Expression *expression_allocate_number(double value)
{
    Expression *expression = calloc(1, sizeof(Expression));
    assert(expression);

    expression->kind = ExpressionKind_Number;
    expression->number.value = value;

    return expression;
}
Expression *expression_allocate_grouping(Expression *expr)
{
    Expression *expression = calloc(1, sizeof(Expression));
    assert(expression);

    expression->kind = ExpressionKind_Grouping;
    expression->grouping.expression = expr;

    return expression;
}

Expression *expression_allocate_negation(Expression *operand)
{
    Expression *expression = calloc(1, sizeof(Expression));
    assert(expression);

    expression->kind = ExpressionKind_Negation;
    expression->negation.operand = operand;

    return expression;
}

Expression *expression_allocate_binary(ExpressionKind kind, Expression *left_operand, Expression *right_operand)
{
    Expression *expression = calloc(1, sizeof(Expression));
    assert(expression);

    expression->kind = kind;
    expression->addition.left_operand = left_operand;
    expression->addition.right_operand = right_operand;

    return expression;
}

void expression_print(Expression *expression)
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
        expression_print(negation.operand);
        printf(")");
    }
    else if (expression->kind == ExpressionKind_Grouping)
    {
        ExpressionGrouping grouping = expression->grouping;
        expression_print(grouping.expression);
    }
    else if (expression->kind == ExpressionKind_Addition)
    {
        ExpressionAddition addition = expression->addition;
        printf("(");
        expression_print(addition.left_operand);
        printf(" + ");
        expression_print(addition.right_operand);
        printf(")");
    }
    else if (expression->kind == ExpressionKind_Subtraction)
    {
        ExpressionSubtraction subtraction = expression->subtraction;
        printf("(");
        expression_print(subtraction.left_operand);
        printf(" - ");
        expression_print(subtraction.right_operand);
        printf(")");
    }
    else if (expression->kind == ExpressionKind_Multiplication)
    {
        ExpressionMultiplication multiplication = expression->multiplication;
        printf("(");
        expression_print(multiplication.left_operand);
        printf(" * ");
        expression_print(multiplication.right_operand);
        printf(")");
    }
    else if (expression->kind == ExpressionKind_Division)
    {
        ExpressionDivision division = expression->division;
        printf("(");
        expression_print(division.left_operand);
        printf(" / ");
        expression_print(division.right_operand);
        printf(")");
    }
    else if (expression->kind == ExpressionKind_Exponentiation)
    {
        ExpressionExponentiation exponentiation = expression->exponentiation;
        printf("(");
        expression_print(exponentiation.left_operand);
        printf(" ^ ");
        expression_print(exponentiation.right_operand);
        printf(")");
    }
    else
    {
        printf("Error: node not supported!");
    }
}
void expression_free(Expression *expression)
{
    if (expression->kind == ExpressionKind_Number)
    {
        ExpressionNumber number = expression->number;
    }
    else if (expression->kind == ExpressionKind_Negation)
    {
        ExpressionNegation negation = expression->negation;
        expression_free(negation.operand);
    }
    else if (expression->kind == ExpressionKind_Grouping)
    {
        ExpressionGrouping grouping = expression->grouping;
        expression_free(grouping.expression);
    }
    else if (expression->kind == ExpressionKind_Addition)
    {
        ExpressionAddition addition = expression->addition;
        expression_free(addition.left_operand);
        expression_free(addition.right_operand);
    }
    else if (expression->kind == ExpressionKind_Subtraction)
    {
        ExpressionSubtraction subtraction = expression->subtraction;
        expression_free(subtraction.left_operand);
        expression_free(subtraction.right_operand);
    }
    else if (expression->kind == ExpressionKind_Multiplication)
    {
        ExpressionMultiplication multiplication = expression->multiplication;
        expression_free(multiplication.left_operand);
        expression_free(multiplication.right_operand);
    }
    else if (expression->kind == ExpressionKind_Division)
    {
        ExpressionDivision division = expression->division;
        expression_free(division.left_operand);
        expression_free(division.right_operand);
    }
    else if (expression->kind == ExpressionKind_Exponentiation)
    {
        ExpressionExponentiation exponentiation = expression->exponentiation;
        expression_free(exponentiation.left_operand);
        expression_free(exponentiation.right_operand);
    }
    else
    {
        printf("warning: Unrecognized node type!");
    }

    free(expression);
}