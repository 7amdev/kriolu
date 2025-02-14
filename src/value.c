#include "kriolu.h"

void value_array_init(ValueArray *values)
{
    values->items = NULL;
    values->count = 0;
    values->capacity = 0;
}

uint32_t value_array_insert(ValueArray *values, Value value)
{
    if (values->capacity < values->count + 1)
    {
        values->capacity = values->capacity < 8 ? 8 : 2 * values->capacity;
        values->items = (Value *)realloc(values->items, values->capacity * sizeof(Value));
        assert(values->items);
    }

    values->items[values->count] = value;
    values->count += 1;
    return values->count - 1;
}

void value_array_free(ValueArray *values)
{
    free(values->items);
    value_array_init(values);
}