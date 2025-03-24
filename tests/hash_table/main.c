#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "kriolu.h"

#define N 10

double get_seconds(void) {
    struct timespec tp = { 0 };
    int ret = timespec_get(&tp, TIME_UTC);
    assert(ret != 0);
    return (double)tp.tv_sec + (double)tp.tv_nsec * 1e-9;
}

// #define PROFILE_BEGIN() 
#define PROFILE_END(label, begin) printf("%s: %lf seconds\n", (label), get_seconds() - (begin))
#define PROFILE(label) for (                                                                \
                         struct t { int i; double begin; } local = {0, get_seconds()};      \
                         local.i < 1;                                                       \
                         ++local.i, PROFILE_END(label, local.begin)                         \
                       )

VirtualMachine g_vm;

enum ValueType {
    Val_Boolean,
    Val_Nil,
    Val_Number,
    Val_String,

    Val_Max
};

void print_value(Value value);

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


    // Randomly init values with different kinds 
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
        printf("Type: (%d, %s) \n", value_type, value_table[value_type]);
    }

    PROFILE("HashTable: Insert New Values")
    {
        // TODO: add assertion
        // TODO: assert that hash-table is adjusting capacity.
        for (int i = 0; i < N; i++) {

            bool is_new = hash_table_set_value(&hash_table, keys[i], values[i]);
            if (is_new) printf("New: ");
            else printf("Replaced a value: ");
            print_value(values[i]);
        }
    }

    PROFILE("HashTable: Update Values")
    {
        // TODO: add assertion
        for (int i = 0; i < N; i++) {

            bool is_new = hash_table_set_value(&hash_table, keys[i], values[i]);
            if (is_new) printf("New: ");
            else printf("Replaced a value: ");
            print_value(values[i]);
        }
    }

    PROFILE("HashTable: Retrieve Values")
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
    PROFILE("HashTable: Delete Values") {}

    // printf("=== Keys\n");
    // for (int i = 0; i < N; i++) {
    //     String key = string_make(keys[i]->characters, keys[i]->length);
    //     printf("i: %d, k: %s, hash: %d\n", i, key.characters, string_hash(key));
    // }

    return 0;
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