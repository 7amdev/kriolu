#ifndef DynamicArray_H
#define DynamicArray_H

#ifndef DynamicArray_Initial_Capacity
#define DynamicArray_Initial_Capacity 256
#endif // DynamicArray_Initial_Capacity

#ifndef DynamicArray_Realloc 
#include <stdlib.h>
#define DynamicArray_Realloc realloc
#endif // DynamicArray_Realloc

#ifndef DynamicArray_Free 
#include <stdlib.h>
#define DynamicArray_Free free
#endif // DynamicArray_Free

#ifndef DynamicArray_Assert 
#include <assert.h>
#define DynamicArray_Assert assert
#endif // DynamicArray_Assert

#define DynamicArrayHeader struct { size_t count; size_t capacity; }

#define DynamicArray_ensure_capacity(d_array, items_count)                                          \
do {                                                                                                \
    size_t new_capacity = (d_array)->count + (items_count);                                         \
    if (new_capacity <= (d_array)->capacity) break;                                                 \
    if ((d_array)->capacity == 0) (d_array)->capacity = DynamicArray_Initial_Capacity;              \
    while (new_capacity > (d_array)->capacity) {                                                    \
        (d_array)->capacity *= 2;                                                                   \
    }                                                                                               \
    (d_array)->items = DynamicArray_Realloc(                                                        \
        (d_array)->items,                                                                           \
        (d_array)->capacity * sizeof(*(d_array)->items)                                             \
    );                                                                                              \
    DynamicArray_Assert((d_array)->items != NULL && "ERROR: Cannot reallocate memory.");            \
} while (0)

#define DynamicArray_append(d_array, item)          \
do {                                                \
    DynamicArray_ensure_capacity((d_array), 1);     \
    (d_array)->items[(d_array)->count] = (item);    \
    (d_array)->count += 1;                          \
} while (0)

#define DynamicArray_append_many(d_array, new_items, new_items_count)   \
do {                                                                    \
    DynamicArray_ensure_capacity((d_array), (new_items_count));         \
    memcpy(                                                             \
        (d_array)->items + (d_array)->count,                            \
        (new_items),                                                    \
        (new_items_count)*sizeof(*(d_array)->items)                     \
    );                                                                  \
    (d_array)->count += new_items_count;                                \
} while (0)

#define DynamicArray_reset(d_array) \
do {                                \
    (d_array)->count = 0;           \
} while (0)

// Usage:
//
//      typedef struct {
//          const char *name;
//          size_t      age;
//      } Person;
//
//      typedef struct {
//          DynamicArray_Members;
//          Person *items;
//      } DynamicArrayPerson;
//
//      DynamicArrayPerson persons = {0};
//      Person p1 = (Person) { "John Doe", 23 };
//      Person p2 = (Person) { "Jane Doe", 27 };
// 
//      DynamicArray_append(&persons, p1);
//      DynamicArray_append(&persons, p2);
//
//      DynamicArray_foreach(Person, &persons, person) {
//          printf("Index:  %zu\n", person.item->i);
//          printf("Name:   %s\n",  person.item->name);
//          printf("Age:    %d\n",  person.item->age);
//          printf("Gender: %d\n",  person.item->gender);
//      }
//
#define DynamicArray_foreach(Type, d_array, it)                             \
    for (                                                                   \
        struct t { Type *item; size_t i; } (it) = { (d_array)->items, 0};   \
        (it).i < (d_array)->count;                                          \
        ( (it).item += 1, (it).i += 1 )                                     \
    )

#define DynamicArray_free(d_array) DynamicArray_Free((d_array)->items)

#endif // DynamicArray_H