#include "kriolu.h"

Expression* expression_allocate(Expression expr)
{
    Expression* expression = (Expression*)calloc(1, sizeof(Expression));
    assert(expression);

    // initialize
    *expression = expr;

    return expression;
}

void expression_print_tree(Expression* expression, int indent)
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
        printf("%s\n", expression_as_boolean(*expression) == true ? "verdadi" : "falsu");
        break;
    }
    case ExpressionKind_Nil:
    {
        printf("nulo\n");
        break;
    }
    case ExpressionKind_String:
    {
        printf("%s\n", expression_as_string(*expression)->characters);
        break;
    }
    case ExpressionKind_Negation:
    {
        Expression* operand = expression_as_negation(*expression).operand;

        printf("-:\n");
        expression_print_tree(operand, indent + 1);
        break;
    }
    case ExpressionKind_Not:
    {
        Expression* operand = expression_as_negation(*expression).operand;

        printf("ka:\n");
        expression_print_tree(operand, indent + 1);
        break;
    }
    case ExpressionKind_Grouping:
    {
        Expression* expr = expression_as_negation(*expression).operand;
        expression_print_tree(expr, indent + 1);
        break;
    }
    case ExpressionKind_Addition:
    case ExpressionKind_Subtraction:
    case ExpressionKind_Multiplication:
    case ExpressionKind_Division:
    case ExpressionKind_Exponentiation:
    case ExpressionKind_Equal_To:
    case ExpressionKind_Greater_Than:
    case ExpressionKind_Less_Than:
    {
        Expression* left_operand = expression_as_binary(*expression).left;
        Expression* right_operand = expression_as_binary(*expression).right;

        if (expression->kind == ExpressionKind_Addition)
            printf("+:\n");
        else if (expression->kind == ExpressionKind_Subtraction)
            printf("-:\n");
        else if (expression->kind == ExpressionKind_Multiplication)
            printf("*:\n");
        else if (expression->kind == ExpressionKind_Division)
            printf("/:\n");
        else if (expression->kind == ExpressionKind_Exponentiation)
            printf("^:\n");
        else if (expression->kind == ExpressionKind_Equal_To)
            printf("==:\n");
        else if (expression->kind == ExpressionKind_Greater_Than)
            printf(">:\n");
        else if (expression->kind == ExpressionKind_Less_Than)
            printf("<:\n");

        expression_print_tree(left_operand, indent + 1);
        expression_print_tree(right_operand, indent + 1);
        break;
    }
    }
}

void expression_print(Expression* expression, int indent) {
    // for (int i = 0; i < indent; i++)
    //     printf(" ");

    switch (expression->kind)
    {
    default:
    {
        // TODO: log Node Type name
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
    case ExpressionKind_String:
    {
        printf("'%s'", expression_as_string(*expression)->characters);
        break;
    }
    case ExpressionKind_Negation:
    {
        Expression* operand = expression_as_negation(*expression).operand;

        printf("(");
        printf("-");
        expression_print(operand, 0);
        printf(")");
        break;
    }
    case ExpressionKind_Not:
    {
        Expression* operand = expression_as_negation(*expression).operand;

        printf("(");
        printf("ka ");
        expression_print(operand, 0);
        printf(")");
        break;
    }
    case ExpressionKind_And:
    {
        Expression* left_operand = expression_as_and(*expression).left;
        Expression* right_operand = expression_as_and(*expression).right;

        printf("(");
        expression_print(left_operand, 0);
        printf(" e ");
        expression_print(right_operand, 0);
        printf(")");
        break;
    }
    case ExpressionKind_Grouping:
    {
        Expression* expr = expression_as_negation(*expression).operand;
        expression_print(expr, 0);
        break;
    }
    case ExpressionKind_Addition:
    case ExpressionKind_Subtraction:
    case ExpressionKind_Multiplication:
    case ExpressionKind_Division:
    case ExpressionKind_Exponentiation:
    case ExpressionKind_Equal_To:
    case ExpressionKind_Greater_Than:
    case ExpressionKind_Greater_Than_Or_Equal_To:
    case ExpressionKind_Less_Than:
    case ExpressionKind_Less_Than_Or_Equal_To:
    {
        Expression* left_operand = expression_as_binary(*expression).left;
        Expression* right_operand = expression_as_binary(*expression).right;

        printf("(");
        expression_print(left_operand, 0);

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
        else if (expression->kind == ExpressionKind_Equal_To)
            printf(" == ");
        else if (expression->kind == ExpressionKind_Greater_Than)
            printf(" > ");
        else if (expression->kind == ExpressionKind_Greater_Than_Or_Equal_To)
            printf(" >= ");
        else if (expression->kind == ExpressionKind_Less_Than)
            printf(" < ");
        else if (expression->kind == ExpressionKind_Less_Than_Or_Equal_To)
            printf(" <= ");

        expression_print(right_operand, 0);
        printf(")");

        break;
    }
    case ExpressionKind_Assignment: {
        printf("<Assignment>\n");
        printf("%*s%s\n", indent + 2, "", expression->as.assignment.lhs->characters);
        printf("%*s", indent + 2, "");
        expression_print(expression->as.assignment.rhs, indent + 2);
        break;
    }
    case ExpressionKind_Variable: {
        printf("(get %s)", expression->as.variable->characters);
        break;
    }
    }
}
void expression_free(Expression* expression)
{

    switch (expression->kind)
    {
        // TODO: add default with assertion
        // TODO: add literal cases
    case ExpressionKind_Negation:
    case ExpressionKind_Grouping:
    case ExpressionKind_Not:
    {
        Expression* operand = expression_as_negation(*expression).operand;
        expression_free(operand);
        break;
    }
    case ExpressionKind_Addition:
    case ExpressionKind_Subtraction:
    case ExpressionKind_Multiplication:
    case ExpressionKind_Division:
    case ExpressionKind_Exponentiation:
    case ExpressionKind_Equal_To:
    case ExpressionKind_Greater_Than:
    case ExpressionKind_Less_Than:
    {
        Expression* left_operand = expression_as_addition(*expression).left;
        Expression* right_operand = expression_as_addition(*expression).right;

        expression_free(left_operand);
        expression_free(right_operand);
        break;
    }
    }

    free(expression);
}