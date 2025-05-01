#include "kriolu.h"

Statement* statement_allocate(Statement value) {
    Statement* statement = (Statement*)calloc(1, sizeof(Statement));
    assert(statement);

    *statement = value;

    return statement;
}

void statement_print(Statement* statement, int indent) {
    int offset = 2;
    int padding_left = indent + offset;
    switch (statement->kind)
    {
    case StatementKind_Expression: {
        printf("<expression statement>\n");
        printf("%*s", padding_left, "");
        expression_print(statement->expression, padding_left);
        // printf("\n");
    } break;
    case StatementKind_Si: {
        printf("<statement si>\n");
        printf("    TBD\n");
    } break;
    case StatementKind_Variable_Declaration: {
        printf("<mimoria>\n");
        printf("%*s%s\n", padding_left, "", statement->variable_declaration.identifier->characters);
        printf("%*s", padding_left, "");
        expression_print(statement->variable_declaration.rhs, 0);
    } break;
    }
}

ArrayStatement* array_statement_allocate()
{
    ArrayStatement* statements = (ArrayStatement*)calloc(1, sizeof(ArrayStatement));
    assert(statements);

    // initialize members
    statements->items = NULL;
    statements->count = 0;
    statements->capacity = 0;

    return statements;
}

uint32_t array_statement_insert(ArrayStatement* statements, Statement statement)
{
    if (statements->capacity < statements->count + 1)
    {
        Statement* items_old = statements->items;
        uint32_t capacity_old = statements->capacity;
        statements->capacity = statements->capacity < 8 ? 8 : 2 * statements->capacity;
        statements->items = (Statement*)calloc(statements->capacity, sizeof(Statement));
        assert(statements->items);
        memmove(statements->items, items_old, capacity_old * sizeof(Statement));
        free(items_old);
    }

    statements->items[statements->count] = statement;
    statements->count += 1;

    return (uint32_t)statements->count - 1;
}

void array_statement_free(ArrayStatement* statements)
{
}