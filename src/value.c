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

void value_print(Value value)
{
    switch (value.kind)
    {
    default:
    {
        assert(false && "Unsupported Value Type. ");
    }
    case Value_Number:
    {
        printf("%g", value_as_number(value));
        break;
    }
    case Value_Boolean:
    {
        printf("%s", value_as_boolean(value) == true ? "true" : "false");
        break;
    }
    case Value_Nil:
    {
        printf("nulo");
        break;
    }
    }
}

void value_array_free(ValueArray *values)
{
    free(values->items);
    value_array_init(values);
}