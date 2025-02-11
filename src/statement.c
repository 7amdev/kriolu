#include "kriolu.h"

StatementArray *statement_array_allocate()
{
    StatementArray *statements = (StatementArray *)calloc(1, sizeof(StatementArray));
    assert(statements);

    // initialize members
    statements->items = NULL;
    statements->count = 0;
    statements->capacity = 0;

    return statements;
}

uint32_t statement_array_insert(StatementArray *statements, Statement statement)
{
    if (statements->capacity < statements->count + 1)
    {
        Statement *items_old = statements->items;
        uint32_t capacity_old = statements->capacity;
        statements->capacity = statements->capacity < 8 ? 8 : 2 * statements->capacity;
        statements->items = (Statement *)calloc(statements->capacity, sizeof(Statement));
        memmove(statements->items, items_old, capacity_old);
        free(items_old);
    }

    statements->items[statements->count] = statement;
    statements->count += 1;

    return (uint32_t)statements->count - 1;
}

void statement_array_free(StatementArray *statements)
{
}