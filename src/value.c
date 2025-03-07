#include "kriolu.h"

bool value_negate_logically(Value value)
{
    // return (
    //     value_is_nil(value) ||
    //     (value_is_boolean(value) && !value_as_boolean(value)));

    if (value_is_nil(value))
        return true;

    if (value_is_boolean(value))
        return !value_as_boolean(value);

    return false;
}

bool value_is_equal(Value a, Value b)
{
    if (a.kind != b.kind)
        return false;

    if (value_is_boolean(a))
        return (value_as_boolean(a) == value_as_boolean(b));
    if (value_is_number(a))
        return value_as_number(a) == value_as_number(b);
    if (value_is_string(a))
    {
        ObjectString *string_a = value_as_string(a);
        ObjectString *string_b = value_as_string(b);
        return (
            string_a->length == string_b->length &&
            memcmp(value_get_string_chars(a), value_get_string_chars(b), string_a->length) == 0);
    }
    if (value_is_nil(a))
        return true;

    return false;
}

inline bool value_is_object_type(Value value, ObjectKind object_kind)
{
    return value_is_object(value) && value_as_object(value)->kind == object_kind;
}

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
        printf("%s", value_as_boolean(value) == true ? "verdadi" : "falsu");
        break;
    }
    case Value_Nil:
    {
        printf("nulo");
        break;
    }
    case Value_Object:
    {
        object_print(value_as_object(value));
        break;
    }
    }
}

void value_array_free(ValueArray *values)
{
    free(values->items);
    value_array_init(values);
}