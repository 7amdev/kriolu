#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "kriolu.h"

void print_value(Value value);
char* value_convert_to_text(Value value);

double get_seconds(void) {
    struct timespec tp = { 0 };
    int ret = timespec_get(&tp, TIME_UTC);
    assert(ret != 0);
    return (double)tp.tv_sec + (double)tp.tv_nsec * 1e-9;
}



//
// Profiler
//

#define PROFILE_END(label, begin) printf("%s: %lf seconds\n", (label), get_seconds() - (begin))
#define PROFILE_BEGIN(label) for (                                                                \
    struct t { int i; double begin; } local = {0, get_seconds()};      \
    local.i < 1;                                                       \
    ++local.i, PROFILE_END(label, local.begin)                         \
)


//
// Logger
//

typedef enum {
    TextColor_Black,
    TextColor_Green,
    TextColor_Red,
    TextColor_Yellow,
    TextColor_White,

    TextColor_Count
} TextColor;

void _log(char* prefix, TextColor color, char* format, ...) {
    va_list args;
    va_start(args, format);
    char buffer[8092] = { 0 };
    vsnprintf(buffer, 8092, format, args);
    va_end(args);

    static char* text_color_table[] = {
        [TextColor_Black] = "\x1b[30m",
        [TextColor_Red] = "\x1b[31m",
        [TextColor_Green] = "\x1b[32m",
        [TextColor_Yellow] = "\x1b[33m",
        [TextColor_White] = "\x1b[97m",
    };

    char log_msg[8092] = { 0 };
    sprintf(log_msg, "%s%s %s \033[0m", text_color_table[color], prefix, buffer);

    printf("%s", log_msg);
}

#define log_trace(format, ...) _log("Trace: ", TextColor_White, format, __VA_ARGS__)
#define log_error(format, ...) _log("Error: ", TextColor_Red, format, __VA_ARGS__)
#define log_success(format, ...) _log("Success: ", TextColor_Green, format, __VA_ARGS__)
#define log_warning(format, ...) _log("Warning: ", TextColor_Yellow, format, __VA_ARGS__)

#ifdef _WIN32
#define debugger_break() __debugbreak();
#elif __linux__
#define debugger_break() __builtin_debugtrap();
#elif __APPLE__
#define debugger_break() __builtin_trap();
#endif

#define assert_that(expression, format, ...)    \
{                                               \
    if (!(expression))                          \
    {                                           \
        log_error(format, __VA_ARGS__);         \
        debugger_break();                       \
    }                                           \
}

//
// Main
//

void init_keys(ObjectString** keys);

#define N 16

VirtualMachine g_vm;

enum ValueType {
    Val_Boolean,
    Val_Nil,
    Val_Number,
    Val_String,

    Val_Max
};


int main(void) {
    HashTable hash_table;
    ObjectString* keys[N] = { 0 };
    Value values[N] = { 0 };
    char* value_table[] = {
        [Val_Boolean] = "Boolean",
        [Val_Nil] = "Nil",
        [Val_Number] = "Number",
        [Val_String] = "String",
    };

    srand(time(NULL));
    hash_table_init(&hash_table);
    hash_table_init(&g_vm.strings);

    // 
    // Initialization
    //

    // Keys
    //
    for (int i = 0; i < N; i++) {
        String string = string_make_from_format("key_%d", i);
        ObjectString* key = object_allocate_string(string.characters, string.length, string_hash(string));
        keys[i] = key;
    }

    // Values
    //
    // for (int i = 0; i < N; i++)
    //     values[i] = value_make_number(i);

    // for (int i = 0; i < N; i++) {
    //     String string = string_make_from_format("value_%d", i);
    //     ObjectString* object_string = object_allocate_string(string.characters, string.length, string_hash(string));
    //     values[i] = value_make_object_string(object_string);
    // }


    // Generate Random values with different types 
    //
    for (int i = 0; i < N; i++)
    {
        Value value = { 0 };
        int value_type = rand() % Val_Max;

        if (value_type == Val_Boolean) {
            value = value_make_boolean(true);
        } else if (value_type == Val_Number) {
            value = value_make_number(i);
        } else if (value_type == Val_Nil) {
            value = value_make_nil();
        } else if (value_type == Val_String) {
            String string = string_make_from_format("value_%d", i);
            ObjectString* object_string = object_allocate_string(string.characters, string.length, string_hash(string));
            value = value_make_object_string(object_string);
        }

        values[i] = value;
    }
    // assert_that(hash_table.capacity == N, "Expected %d generated values, bu instead got %d.", N, hash_table.count);

    int count = 0;
    PROFILE_BEGIN("==== Insert New Values")
    {
        for (int i = 0; i < N; i++) {
            bool is_new = hash_table_set_value(&hash_table, keys[i], values[i]);
            if (is_new) count += 1;;
        }

    }
    assert_that(count == hash_table.count, "Expected %d new inserts, but instead got %d.\n", N, count);

    for (int i = 0; i < N; i++) {
        ObjectString* key = keys[i];
        Value value = { 0 };

        bool found = hash_table_get_value(&hash_table, key, &value);
        bool is_equal = value_is_equal(values[i], value);
        assert_that(found, "Expected value '%s' for key '%s', but got nothing.", value_convert_to_text(values[i]), key->characters);
        assert_that(is_equal, "Expected value '%s' for key '%s', but instead got '%s'.", value_convert_to_text(values[i]), key->characters, value_convert_to_text(value));
    }

    for (int i = 0; i < N; i++) {
        String string = string_make_from_format("key_%d", i);
        ObjectString* key = object_allocate_string(string.characters, string.length, string_hash(string));
        keys[i] = key;
    }
    for (int i = 0; i < N; i++) {
        ObjectString* key = keys[i];
        Value value = { 0 };
        // Generate an ObjectString 'key_0' and see if entry->key == key?
        // Probably not because we dont have deduplication.
        bool found = hash_table_get_value(&hash_table, key, &value);
        bool is_equal = value_is_equal(values[i], value);
        assert_that(found, "Expected value '%s' for key '%s', but got nothing.", value_convert_to_text(values[i]), key->characters);
        assert_that(is_equal, "Expected value '%s' for key '%s', but instead got '%s'.", value_convert_to_text(values[i]), key->characters, value_convert_to_text(value));
    }

    PROFILE_BEGIN("HashTable: Update Values")
    {
        // TODO: add assertion
        for (int i = 0; i < N; i++) {
            bool is_new = hash_table_set_value(&hash_table, keys[i], values[i]);
            if (is_new) printf("New: ");
            else printf("Replaced a value: ");
            print_value(values[i]);
        }
    }

    PROFILE_BEGIN("HashTable: Retrieve Values")
    {
        // TODO: add assertion
        for (int i = 0; i < N; i++) {
            Value value = { 0 };
            if (hash_table_get_value(&hash_table, keys[i], &value)) {
                printf("Value: ");
                print_value(value);
            } else {
                printf("Error: key %s not found.", keys[i]->characters);
            }
        }
    }

    // TODO: delete
    PROFILE_BEGIN("HashTable: Delete Values") {}

    // printf("=== Keys\n");
    // for (int i = 0; i < N; i++) {
    //     String key = string_make(keys[i]->characters, keys[i]->length);
    //     printf("i: %d, k: %s, hash: %d\n", i, key.characters, string_hash(key));
    // }

    return 0;
}

void init_keys(ObjectString** keys) {
    for (int i = 0; i < N; i++) {
        String string = string_make_from_format("key_%d", i);
        ObjectString* key = object_allocate_string(string.characters, string.length, string_hash(string));
        keys[i] = key;
    }
}

char* value_convert_to_text(Value value) {
    char* value_text_buffer = (char*)calloc(8190, sizeof(char));
    assert(value_text_buffer);

    switch (value.kind)
    {
    default: sprintf(value_text_buffer, "Value Kind is not supported"); break;
    case Value_Boolean: {
        sprintf(value_text_buffer, "%s", value_as_boolean(value) ? "true" : "false");
    } break;
    case Value_Number: {
        sprintf(value_text_buffer, "%lf", value_as_number(value));
    } break;
    case Value_Nil: {
        sprintf(value_text_buffer, "nil");
    } break;
    case Value_Object: {
        if (value_is_string(value)) {
            sprintf(value_text_buffer, "%s", value_as_string(value)->characters);
        }
    } break;
    }

    return value_text_buffer;
}

void print_value(Value value) {
    switch (value.kind)
    {
    default: printf("Value Kind is not supported\n"); break;
    case Value_Boolean: {
        printf("%s\n", value_as_boolean(value) ? "true" : "false");
    } break;
    case Value_Number: {
        printf("%lf\n", value_as_number(value));
    } break;
    case Value_Nil: {
        printf("nil\n");
    } break;
    case Value_Object: {
        if (value_is_string(value)) {
            printf("%s\n", value_as_string(value)->characters);
        }
    } break;
    }
}