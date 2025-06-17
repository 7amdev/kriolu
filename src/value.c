#include "kriolu.h"


bool value_negate_logically(Value value) {
    // return (
    //     value_is_nil(value) ||
    //     (value_is_boolean(value) && !value_as_boolean(value)));

    if (value_is_nil(value)) return true;
    if (value_is_boolean(value)) return !value_as_boolean(value);

    // Other Values
    //
    return false;
}

// Falsey values: nil, Boolean(false)
// Truthy values: Boolean(true), String, Number, Object
//
bool value_is_falsey(Value value) {
    if (value_is_nil(value)) return true;
    if (value_is_boolean(value) && value_as_boolean(value) == false) return true;

    return false;
}

bool value_is_equal(Value a, Value b) {
    if (a.kind != b.kind)    return false;
    if (value_is_boolean(a)) return (value_as_boolean(a) == value_as_boolean(b));
    if (value_is_number(a))  return value_as_number(a) == value_as_number(b);
    if (value_is_nil(a))     return true;
    if (value_is_object(a))  return value_as_object(a) == value_as_object(b);
    // if (value_is_string(a)) {
    //     ObjectString* string_a = value_as_string(a);
    //     ObjectString* string_b = value_as_string(b);
    //     return (
    //         string_a->length == string_b->length &&
    //         memcmp(value_get_string_chars(a), value_get_string_chars(b), string_a->length) == 0
    //         );
    // }

    return false;
}

void array_value_init(ArrayValue* values)
{
    values->items = NULL;
    values->count = 0;
    values->capacity = 0;
}

uint32_t array_value_insert(ArrayValue* values, Value value)
{
    if (values->capacity < values->count + 1) {
        values->capacity = values->capacity < 8 ? 8 : 2 * values->capacity;
        values->items = (Value*)realloc(values->items, values->capacity * sizeof(Value));
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
        Object_print(value_as_object(value));
        break;
    }
    }
}

void array_value_free(ArrayValue* values)
{
    free(values->items);
    array_value_init(values);
}