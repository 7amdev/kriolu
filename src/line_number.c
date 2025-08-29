#include "kriolu.h"

void array_line_init(ArrayLineNumber* lines) {
    lines->items    = NULL;
    lines->count    = 0;
    lines->capacity = 0;
}

int array_line_insert(ArrayLineNumber* lines, int line) {
    if (lines->capacity < lines->count + 1) {
        int old_capacity = lines->capacity;
        lines->capacity = lines->capacity < 8 ? 8 : 2 * lines->capacity;
        lines->items = Memory_AllocateArray(int, lines->items, old_capacity, lines->capacity);
        assert(lines->items);
        for (int i = lines->capacity - 1; i >= lines->count; --i) {
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
int array_line_insert_3x(ArrayLineNumber* lines, int line) {
    array_line_insert(lines, line);
    array_line_insert(lines, line);
    array_line_insert(lines, line);

    return lines->count - 1;
}

void array_line_free(ArrayLineNumber* lines) {
    free(lines->items);
    array_line_init(lines);
}