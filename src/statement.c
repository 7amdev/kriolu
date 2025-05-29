#include "kriolu.h"

char* g_statement_kind_text[StatementKind_Max] = {
    [StatementKind_Invalid] = "Invalid Statement!",
    [StatementKind_Expression] = "Statment::Expression",
    [StatementKind_Variable_Declaration] = "Statement::Variable declaration",
    [StatementKind_Print] = "Statment::Imprimi",
    [StatementKind_Block] = "Statement::Block",
    [StatementKind_Return] = "Statement::Divolvi",
    [StatementKind_Si] = "Statement::Si",
    [StatementKind_Di] = "Statement::Di",
    [StatementKind_Timenti] = "Statement::Timenti",
};

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
    default: {
        printf("%s not supported\n", g_statement_kind_text[statement->kind]);
    } break;
    case StatementKind_Expression: {
        printf("<expression statement>\n");
        printf("%*s", padding_left, "");
        expression_print(statement->expression, padding_left);
    } break;
    case StatementKind_Si: {
        printf("<statement si>\n");
        printf("<condition>\n");
        printf("%*s", padding_left, "");
        expression_print(statement->_if.condition, padding_left);
        printf("\n<then>\n");
        printf("%*s", padding_left, "");
        statement_print(statement->_if.then_block, padding_left);
        printf("\n<else>\n");
        printf("%*s", padding_left, "");
        statement_print(statement->_if.else_block, padding_left);
    } break;
    case StatementKind_Block: {
        printf("<block>\n");
        printf("%*s", padding_left, "");
        for (int i = 0; i < statement->bloco->count; i++) {
            statement_print(&statement->bloco->items[i], padding_left);
            if ((i + 1) < statement->bloco->count) {
                printf("\n");
                printf("%*s", padding_left, "");
            }
        }
    } break;
    case StatementKind_Variable_Declaration: {
        printf("<mimoria>\n");
        printf("%*s%s\n", padding_left, "", statement->variable_declaration.identifier->characters);
        printf("%*s", padding_left, "");
        expression_print(statement->variable_declaration.rhs, 0);
    } break;
    case StatementKind_Print: {
        printf("<imprimi>\n");
        printf("%*s", padding_left, "");
        expression_print(statement->print, padding_left);
    } break;
    case StatementKind_Timenti: {
        printf("<timenti>\n");
        printf("%*s", padding_left, "");
        printf("<condition>\n");
        printf("%*s", padding_left, "");
        printf("%*s", padding_left, "");
        expression_print(statement->timenti.condition, padding_left);
        printf("\n");
        printf("%*s", padding_left, "");
        statement_print(statement->timenti.body, padding_left);
    } break;
    case StatementKind_Pa: {
        printf("<pa>\n");
        printf("%*s", padding_left, "");
        printf("<initializer>\n");
        printf("%*s", padding_left, "");
        printf("%*s", padding_left, "");
        statement_print(statement->pa.initializer, 2 * padding_left);
        printf("\n");
        printf("%*s", padding_left, "");
        printf("<condition>\n");
        printf("%*s", padding_left, "");
        printf("%*s", padding_left, "");
        expression_print(statement->pa.condition, 2 * padding_left);
        printf("\n");
        printf("%*s", padding_left, "");
        printf("<increment>\n");
        printf("%*s", padding_left, "");
        printf("%*s", padding_left, "");
        expression_print(statement->pa.increment, 2 * padding_left);
        printf("\n");
        printf("%*s", padding_left, "");
        printf("<body>\n");
        printf("%*s", padding_left, "");
        printf("%*s", padding_left, "");
        statement_print(statement->pa.body, 2 * padding_left);
    } break;
    case StatementKind_Sai: {
        // TODO
        printf("<sai>\n");
    } break;
    case StatementKind_Salta: {
        // TODO
        printf("<salta>\n");
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