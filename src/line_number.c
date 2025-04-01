#include "kriolu.h"

void line_array_init(ArrayLineNumber* lines)
{
    lines->items = NULL;
    lines->count = 0;
    lines->capacity = 0;
}

int line_array_insert(ArrayLineNumber* lines, int line)
{
    if (lines->capacity < lines->count + 1)
    {
        lines->capacity = lines->capacity < 8 ? 8 : 2 * lines->capacity;
        lines->items = (int*)realloc(lines->items, lines->capacity * sizeof(int));
        assert(lines->items);
        for (int i = lines->capacity - 1; i >= lines->count; --i)
        {
            lines->items[i] = -1;
        }
    }

    lines->items[lines->count] = line;
    lines->count += 1;

    return lines->count - 1;
}

// For 24bit intruction operand, we need to insert 3x the
// number of lines to match the instructions array index, otherwise
// the debugger print line number functionality will be unaligned.
//
int line_array_insert_3x(ArrayLineNumber* lines, int line)
{
    line_array_insert(lines, line);
    line_array_insert(lines, line);
    line_array_insert(lines, line);

    return lines->count - 1;
}

void line_array_free(ArrayLineNumber* lines)
{
    free(lines->items);
    line_array_init(lines);
}