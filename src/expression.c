#include "kriolu.h"

Expression *expression_allocate(Expression expr)
{
    Expression *expression = (Expression *)calloc(1, sizeof(Expression));
    assert(expression);

    // initialize
    *expression = expr;

    return expression;
}

void expression_print_tree(Expression *expression, int indent)
{
    for (int i = 0; i < indent; i++)
        printf("  ");

    switch (expression->kind)
    {
    default:
    {
        assert(false && "Node not supported.");
        break;
    }
    case ExpressionKind_Number:
    {
        double number = expression_as_number(*expression);
        printf("%.1f\n", number);
        break;
    }
    case ExpressionKind_Boolean:
    {
        printf("%s\n", expression_as_boolean(*expression) == true ? "true" : "false");
        break;
    }
    case ExpressionKind_Nil:
    {
        printf("nulo\n");
        break;
    }
    case ExpressionKind_Negation:
    {
        Expression *operand = expression_as_negation(*expression).operand;

        printf("-:\n");
        expression_print_tree(operand, indent + 1);
        break;
    }
    case ExpressionKind_Not:
    {
        Expression *operand = expression_as_negation(*expression).operand;

        printf("ka:\n");
        expression_print_tree(operand, indent + 1);
        break;
    }
    case ExpressionKind_Grouping:
    {
        Expression *expr = expression_as_negation(*expression).operand;
        expression_print_tree(expr, indent + 1);
        break;
    }
    case ExpressionKind_Addition:
    case ExpressionKind_Subtraction:
    case ExpressionKind_Multiplication:
    case ExpressionKind_Division:
    case ExpressionKind_Exponentiation:
    {
        Expression *left_operand = expression_as_binary(*expression).left;
        Expression *right_operand = expression_as_binary(*expression).right;

        printf("+:\n");
        expression_print_tree(left_operand, indent + 1);
        expression_print_tree(right_operand, indent + 1);
        break;
    }
    }
}

void expression_print(Expression *expression)
{

    switch (expression->kind)
    {
    default:
    {
        assert(false && "Node not supported.");
        break;
    }
    case ExpressionKind_Number:
    {
        double number = expression_as_number(*expression);
        printf("%.1f", number);
        break;
    }
    case ExpressionKind_Boolean:
    {
        printf("%s", expression_as_boolean(*expression) == true ? "verdadi" : "falsu");
        break;
    }
    case ExpressionKind_Nil:
    {
        printf("nulo");
        break;
    }
    case ExpressionKind_Negation:
    {
        Expression *operand = expression_as_negation(*expression).operand;

        printf("(");
        printf("-");
        expression_print(operand);
        printf(")");
        break;
    }
    case ExpressionKind_Not:
    {
        Expression *operand = expression_as_negation(*expression).operand;

        printf("(");
        printf("ka ");
        expression_print(operand);
        printf(")");
        break;
    }
    case ExpressionKind_Grouping:
    {
        Expression *expr = expression_as_negation(*expression).operand;
        expression_print(expr);
        break;
    }
    case ExpressionKind_Addition:
    case ExpressionKind_Subtraction:
    case ExpressionKind_Multiplication:
    case ExpressionKind_Division:
    case ExpressionKind_Exponentiation:
    {
        Expression *left_operand = expression_as_binary(*expression).left;
        Expression *right_operand = expression_as_binary(*expression).right;

        printf("(");
        expression_print(left_operand);

        if (expression->kind == ExpressionKind_Addition)
            printf(" + ");
        else if (expression->kind == ExpressionKind_Subtraction)
            printf(" - ");
        else if (expression->kind == ExpressionKind_Multiplication)
            printf(" * ");
        else if (expression->kind == ExpressionKind_Division)
            printf(" / ");
        else if (expression->kind == ExpressionKind_Exponentiation)
            printf(" ^ ");

        expression_print(right_operand);
        printf(")");

        break;
    }
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