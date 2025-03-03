#include "kriolu.h"

Expression *expression_allocate(Expression expr)
{
    Expression *expression = (Expression *)calloc(1, sizeof(Expression));
    assert(expression);

    // init
    *expression = expr;

    return expression;
}

Expression *expression_allocate_value(Value value)
{
    Expression *expression = calloc(1, sizeof(Expression));
    assert(expression);

    expression->kind = ExpressionKind_Value;
    expression->as.value = value;

    return expression;
}

Expression *expression_allocate_grouping(Expression *expr)
{
    Expression *expression = calloc(1, sizeof(Expression));
    assert(expression);

    expression->kind = ExpressionKind_Grouping;
    expression->as.unary.operand = expr;

    return expression;
}

Expression *expression_allocate_negation(Expression *operand)
{
    Expression *expression = calloc(1, sizeof(Expression));
    assert(expression);

    expression->kind = ExpressionKind_Negation;
    expression->as.unary.operand = operand;

    return expression;
}

Expression *expression_allocate_binary(ExpressionKind kind, Expression *left_operand, Expression *right_operand)
{
    Expression *expression = calloc(1, sizeof(Expression));
    assert(expression);

    expression->kind = kind;
    expression->as.binary.left = left_operand;
    expression->as.binary.right = right_operand;

    return expression;
}

void expression_print_tree(Expression *expression, int indent)
{
    for (int i = 0; i < indent; i++)
        printf("  ");

    if (expression->kind == ExpressionKind_Value)
    {
        Value value = expression_as_value(*expression);
        if (value_is_number(value))
        {
            double number = value_as_number(value);
            printf("%.1f\n", number);
        }
        else if (value_is_boolean(value))
        {
            printf("%s\n", value_as_boolean(value) == true ? "true" : "false");
        }
        else if (value_is_nil(value))
        {
            printf("nulo\n");
        }
    }
    else if (expression->kind == ExpressionKind_Negation)
    {
        Expression *operand = expression_as_negation(*expression).operand;

        printf("-:\n");
        expression_print_tree(operand, indent + 1);
    }
    else if (expression->kind == ExpressionKind_Grouping)
    {
        Expression *expr = expression_as_negation(*expression).operand;
        expression_print_tree(expr, indent + 1);
    }
    else if (expression->kind == ExpressionKind_Addition)
    {
        Expression *left_operand = expression_as_addition(*expression).left;
        Expression *right_operand = expression_as_addition(*expression).right;

        printf("+:\n");
        expression_print_tree(left_operand, indent + 1);
        expression_print_tree(right_operand, indent + 1);
    }
    else if (expression->kind == ExpressionKind_Subtraction)
    {
        Expression *left_operand = expression_as_subtraction(*expression).left;
        Expression *right_operand = expression_as_subtraction(*expression).right;

        printf("-:\n");
        expression_print_tree(left_operand, indent + 1);
        expression_print_tree(left_operand, indent + 1);
    }
    else if (expression->kind == ExpressionKind_Multiplication)
    {
        Expression *left_operand = expression_as_multiplication(*expression).left;
        Expression *right_operand = expression_as_multiplication(*expression).right;

        printf("*:\n");
        expression_print_tree(left_operand, indent + 1);
        expression_print_tree(right_operand, indent + 1);
    }
    else if (expression->kind == ExpressionKind_Division)
    {
        Expression *left_operand = expression_as_division(*expression).left;
        Expression *right_operand = expression_as_division(*expression).right;

        printf("/:\n");
        expression_print_tree(left_operand, indent + 1);
        expression_print_tree(right_operand, indent + 1);
    }
    else if (expression->kind == ExpressionKind_Exponentiation)
    {
        Expression *left_operand = expression_as_exponentiation(*expression).left;
        Expression *right_operand = expression_as_exponentiation(*expression).right;

        printf("^:\n");
        expression_print_tree(left_operand, indent + 1);
        expression_print_tree(right_operand, indent + 1);
    }
    else
    {
        printf("Error: node not supported!");
    }
}

void expression_print(Expression *expression)
{

    // TODO: im not creating literal expression Nil, or Boolean correctly
    if (expression->kind == ExpressionKind_Value)
    {
        Value value = expression_as_value(*expression);
        if (value_is_number(value))
        {
            double number = value_as_number(value);
            printf("%.1f", number);
        }
        else if (value_is_boolean(value))
        {
            printf("%s", value_as_boolean(value) == true ? "true" : "false");
        }
        else if (value_is_nil(value))
        {
            printf("nulo");
        }
    }
    else if (expression->kind == ExpressionKind_Negation)
    {
        Expression *operand = expression_as_negation(*expression).operand;

        printf("(");
        printf("-");
        expression_print(operand);
        printf(")");
    }
    else if (expression->kind == ExpressionKind_Grouping)
    {
        Expression *expr = expression_as_negation(*expression).operand;
        expression_print(expr);
    }
    else if (expression->kind == ExpressionKind_Addition)
    {
        Expression *left_operand = expression_as_addition(*expression).left;
        Expression *right_operand = expression_as_addition(*expression).right;

        printf("(");
        expression_print(left_operand);
        printf(" + ");
        expression_print(right_operand);
        printf(")");
    }
    else if (expression->kind == ExpressionKind_Subtraction)
    {
        Expression *left_operand = expression_as_subtraction(*expression).left;
        Expression *right_operand = expression_as_subtraction(*expression).right;

        printf("(");
        expression_print(left_operand);
        printf(" - ");
        expression_print(right_operand);
        printf(")");
    }
    else if (expression->kind == ExpressionKind_Multiplication)
    {
        Expression *left_operand = expression_as_multiplication(*expression).left;
        Expression *right_operand = expression_as_multiplication(*expression).right;

        printf("(");
        expression_print(left_operand);
        printf(" * ");
        expression_print(right_operand);
        printf(")");
    }
    else if (expression->kind == ExpressionKind_Division)
    {
        Expression *left_operand = expression_as_division(*expression).left;
        Expression *right_operand = expression_as_division(*expression).right;

        printf("(");
        expression_print(left_operand);
        printf(" / ");
        expression_print(right_operand);
        printf(")");
    }
    else if (expression->kind == ExpressionKind_Exponentiation)
    {
        Expression *left_operand = expression_as_exponentiation(*expression).left;
        Expression *right_operand = expression_as_exponentiation(*expression).right;

        printf("(");
        expression_print(left_operand);
        printf(" ^ ");
        expression_print(right_operand);
        printf(")");
    }
    else
    {
        printf("Error: node not supported!");
    }
}
void expression_free(Expression *expression)
{

    switch (expression->kind)
    {
    case ExpressionKind_Negation:
    case ExpressionKind_Grouping:
    {
        Expression *operand = expression_as_negation(*expression).operand;
        expression_free(operand);
        break;
    }
    case ExpressionKind_Addition:
    case ExpressionKind_Subtraction:
    case ExpressionKind_Multiplication:
    case ExpressionKind_Division:
    case ExpressionKind_Exponentiation:
    {
        Expression *left_operand = expression_as_addition(*expression).left;
        Expression *right_operand = expression_as_addition(*expression).right;

        expression_free(left_operand);
        expression_free(right_operand);
        break;
    }
    }

    free(expression);
}