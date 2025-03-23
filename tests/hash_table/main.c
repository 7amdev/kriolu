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
#define PROFILE(label) for (                                                            \
                         struct { int i; double begin; } data = {0, get_seconds()};     \
                         data.i < 1;                                                    \
                         ++data.i, PROFILE_END(label, data.begin)                       \
                       )

VirtualMachine g_vm;

// TODO: Mr. zozin profiling technique

int main(void) {
    String keys[N] = { 0 };
    Value values[N] = { 0 };


    srand(time(NULL));

    for (int i = 0; i < N; i++)
        keys[i] = string_make_from_format("key_%d", i);

    // 
    // Initialization
    //

    // Uncomment for test
    //
    // for (int i = 0; i < N; i++)
    //     values[i] = value_make_number(i);

    for (int i = 0; i < N; i++) {
        String string = string_make_from_format("value_%d", i);
        ObjectString* object_string = object_allocate_string(string.characters, string.length, string_hash(string));
        values[i] = value_make_object_string(object_string);
    }

    // TODO: insert values into hash table and profile
    // TODO: remove values from hash table and profile
    // TODO: update values in the hash table and profile
    // TODO: read values from hash table and profile

    PROFILE("Profiling Inserting items in Hash Table")
    {
        int x = 2;
        for (int i = 0; i < 100000000; i++)
            x *= i;
    }

    // Randomly init values with different kinds 
    //
    for (int i = 0; i < N; i++)
    {
        int value_type = rand() % Value_Count;

        // TODO: incomplete 
        Value value = { 0 };
        if (value_type == Value_Boolean) {
            value = value_make_boolean(true);
        } else if (value_type == Value_Number) {
            value = value_make_number(i);
        }

        values[i] = value;
        printf("rand: %d\n", value_type);
    }


    printf("=== Keys\n");
    for (int i = 0; i < N; i++)
        printf("i: %d, k: %s, hash: %d\n", i, keys[i].characters, string_hash(keys[i]));

    printf("=== Values\n");
    for (int i = 0; i < N; i++)
    {
        Value value = values[i];
        switch (value.kind)
        {
        default: printf("Value Kind is not supported\n"); break;
        case Value_Number: {
            printf("i: %d, v: %lf\n", i, value_as_number(values[i]));
        } break;
        case Value_Nil: {
            printf("i: %d, v: nil\n", i);
        } break;
        case Value_Object: {
            if (value_is_string(value)) {
                printf("i: %d, v: %s\n", i, value_as_string(value)->characters);
            }
        } break;
        }
    }


    return 0;
}
