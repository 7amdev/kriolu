#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "kriolu.h"


//
// Profiler
//

double get_seconds(void) {
    struct timespec tp = { 0 };
    int ret = timespec_get(&tp, TIME_UTC);
    assert(ret != 0);
    return (double)tp.tv_sec + (double)tp.tv_nsec * 1e-9;
}

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
    sprintf(log_msg, "%s%s %s\033[0m", text_color_table[color], prefix, buffer);

    printf("%s", log_msg);
}

#define log_trace(format, ...) _log("Trace: ", TextColor_White, format, __VA_ARGS__)
#define log_error(format, ...) _log("Error: ", TextColor_Red, format, __VA_ARGS__)
#define log_success(format, ...) _log("Success: ", TextColor_Green, format, __VA_ARGS__)
#define log_passed(format, ...) _log("PASSED: ", TextColor_Green, format, __VA_ARGS__)
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

void generate_keys(ObjectString** keys);
void generate_and_intern_keys(HashTable* table, ObjectString** keys, int len);
void generate_string_values(Value* values);
void generate_number_values(Value* values);
void generate_random_values(Value* values, int len);
void print_value(Value value);
char* value_convert_to_text(Value value);

#define N 29

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
    // hash_table_init(&g_vm.string_database);

    // 
    // Initialization
    //

    // Keys
    //
    generate_keys(keys);

    // Generate Random values with different types 
    //
    generate_random_values(values, N);

    //
    // Tests
    //

    // Insert values
    //
    int count = 0;
    PROFILE_BEGIN("Profiler: Insert New Values")
    {
        // TODO: calculate capacity and the assert at the end
        for (int i = 0; i < N; i++) {
            bool is_new = hash_table_set_value(&hash_table, keys[i], values[i]);
            if (is_new) count += 1;;
        }
    }
    assert_that(count == hash_table.count, "Expected %d new inserts, but instead got %d.\n", N, count);
    log_passed(" Insert New Values\n", NULL);

    // Get values 
    //
    for (int i = 0; i < N; i++) {
        ObjectString* key = keys[i];
        Value value = { 0 };

        bool found = hash_table_get_value(&hash_table, key, &value);
        bool is_equal = value_is_equal(values[i], value);
        assert_that(found, "Expected value '%s' for key '%s', but got nothing.\n", value_convert_to_text(values[i]), key->characters);
        assert_that(is_equal, "Expected value '%s' for key '%s', but instead got '%s'.\n", value_convert_to_text(values[i]), key->characters, value_convert_to_text(value));
    }
    log_passed(" Get Values\n", NULL);


    // TEST: Update values
    //
    Value update_values[N] = { 0 };
    generate_random_values(update_values, N);
    int update_values_count = 0;
    PROFILE_BEGIN("Profiler: Update Values")
    {
        for (int i = 0; i < N; i++) {
            Value old_value = { 0 };
            bool found_old_value = hash_table_get_value(&hash_table, keys[i], &old_value);
            bool is_new = hash_table_set_value(&hash_table, keys[i], update_values[i]);
            if (!is_new) update_values_count += 1;

            assert_that(found_old_value, "Expected value '%s' for key '%s', but got nothing.\n", value_convert_to_text(values[i]), keys[i]->characters);
            assert_that(is_new == false, "Expected to replace value '%s' for key '%s', but the entry was empty.\n", value_convert_to_text(old_value), keys[i]->characters);
        }
    }
    assert_that(update_values_count == N, "Expected %d updates, but instead got %d.\n", N, update_values_count);

    for (int i = 0; i < N; i++) {
        ObjectString* key = keys[i];
        Value value = { 0 };

        bool found = hash_table_get_value(&hash_table, key, &value);
        bool is_equal = value_is_equal(update_values[i], value);
        assert_that(found, "Expected value '%s' for key '%s', but got nothing.\n", value_convert_to_text(values[i]), key->characters);
        assert_that(is_equal, "Expected value '%s' for key '%s', but instead got '%s'.\n", value_convert_to_text(values[i]), key->characters, value_convert_to_text(value));
    }
    log_passed(" Update Values\n", NULL);

    // TEST: Delete values
    //
    int key_index = rand() % N;
    ObjectString* key = keys[key_index];
    Value value = { 0 };
    assert_that(hash_table_get_value(&hash_table, key, &value), "Could not find value for key '%s'.", key->characters);
    assert_that(hash_table_delete(&hash_table, key), "Could not find value for key '%s'.", key->characters);
    assert_that(hash_table_get_value(&hash_table, key, &value) == false, "Expected a tombstone, but instead got a diferent value", NULL);
    log_passed(" Delete Value\n", NULL);

    // Deduplication
    // On function hash_table_find_entry_by_key, the check 'entry->key == key' should work 
    //
    ObjectString* deduplicaton_keys[N] = { 0 };
    Value deduplication_values[N] = { 0 };
    HashTable keys_db;
    HashTable key_value_table;

    hash_table_init(&keys_db);
    hash_table_init(&key_value_table);
    generate_and_intern_keys(&keys_db, deduplicaton_keys, N);
    generate_random_values(deduplication_values, N);

    PROFILE_BEGIN("Profiler: Deduplication -> Insert New Values")
    {
        for (int i = 0; i < N; i++) {
            bool is_new = hash_table_set_value(&key_value_table, deduplicaton_keys[i], deduplication_values[i]);
            if (is_new) count += 1;;
        }
    }
    assert_that(key_value_table.count == N, "Expected %d new inserts, but instead got %d.\n", N, key_value_table);
    assert_that(keys_db.count == N, "Expected %d generated keys, but got %d.", N, keys_db.count);

    // Generates the same key's name, length, and hash, but in a different 
    // memory location making expression such as 'entry->key == key' invalid.
    //
    generate_keys(deduplicaton_keys);

    for (int i = 0; i < N; i++) {
        ObjectString* dedup_key = deduplicaton_keys[i];
        String string_key = ObjectString_to_string(dedup_key);
        Value dedup_value = { 0 };
        ObjectString* key_in_database = hash_table_get_key(&keys_db, string_key, string_hash(string_key));
        if (key_in_database != NULL) dedup_key = key_in_database;

        bool found = hash_table_get_value(&key_value_table, dedup_key, &dedup_value);
        bool is_equal = value_is_equal(deduplication_values[i], dedup_value);

        assert_that(found, "Expected value '%s' for key '%s', but got nothing.\n", value_convert_to_text(deduplication_values[i]), dedup_key->characters);
        assert_that(is_equal, "Expected value '%s' for key '%s', but instead got '%s'.\n", value_convert_to_text(deduplication_values[i]), dedup_key->characters, value_convert_to_text(value));
    }
    log_passed(" Deduplication\n", NULL);

    return 0;
}

void generate_keys(ObjectString** keys) {
    for (int i = 0; i < N; i++) {
        String string = string_make_from_format("key_%d", i);
        ObjectString* key = ObjectString_allocate(string.characters, string.length, string_hash(string));
        keys[i] = key;
    }
}

void generate_and_intern_keys(HashTable* table, ObjectString** keys, int len)
{
    for (int i = 0; i < len; i++)
    {
        String string = string_make_from_format("key_%d", i);
        ObjectString* key = ObjectString_allocate_and_intern(table, string.characters, string.length, string_hash(string));
        keys[i] = key;
    }
}

void generate_random_values(Value* values, int len) {
    for (int i = 0; i < len; i++)
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
            ObjectString* object_string = ObjectString_allocate(string.characters, string.length, string_hash(string));
            value = value_make_object_string(object_string);
        }

        values[i] = value;
    }
}

void generate_string_values(Value* values) {
    for (int i = 0; i < N; i++) {
        String string = string_make_from_format("value_%d", i);
        ObjectString* object_string = ObjectString_allocate(string.characters, string.length, string_hash(string));
        values[i] = value_make_object_string(object_string);
    }
}

void generate_number_values(Value* values) {
    for (int i = 0; i < N; i++)
        values[i] = value_make_number(i);
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