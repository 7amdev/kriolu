#include "kriolu.h"

Object *object_allocate(ObjectKind kind, size_t size)
{
    Object *object = (Object *)calloc(1, sizeof(Object));
    assert(object);

    object_init(object, kind);
    return object;
}

// TODO: add Object *next as a parameter to be initialized
//
void object_init(Object *object, ObjectKind kind)
{
    object->kind = kind;

    // Linked List
    // g_vm.objects -> NULL
    // g_vm.objects -> object_1.next -> NULL
    //
    // object.next -> object_1
    // g_vm.objects -> object.next -> object_1.next -> NULL
    //
    // TODO: change g_vm.objects to g_objects declared in object.c
    //       to maintain encapsulation
    //
    object->next = g_vm.objects;
    g_vm.objects = object;
}

void object_clear(Object *object)
{
    object->kind = ObjectKind_Invalid;
}

void object_print(Object *object)
{
    switch (object->kind)
    {
    case ObjectKind_String:
    {
        ObjectString *string = ((ObjectString *)object);
        printf("%.10s", string->characters);
        if (string->length > 10)
            printf("...");

        break;
    }
    }
}

void object_free(Object *object)
{
    switch (object->kind)
    {
    case ObjectKind_String:
    {
        // TODO: update virtual machine's bytes allocated variable
        object_free_string((ObjectString *)object);
        break;
    }
    }
}

// TODO: change the parameters to: object_allocate_string(String string)
//
ObjectString *object_allocate_string(char *characters, int length)
{
    // 1. Copy string from source file to heap
    //
    // 2. Allocate Object with ObjectString type size and initialize
    //    its type to ObjectKind_String
    //
    // 3. Cast Object to ObjectString and set member character pointing to the string
    //    allocated in the heap
    //
    // TODO: Where to put this code? Should a create a type ArrayChar
    //       char *characters = malloc(sizeof(char) * length + 1);
    //       assert(characters);
    //       memcpy(characters, start, length);
    //       characters[length] = '\0';

    ObjectString *string = calloc(1, sizeof(ObjectString));
    assert(string);

    object_init((Object *)string, ObjectKind_String);
    assert(string->object.kind == ObjectKind_String);
    string->characters = characters;
    string->length = length;

    return string;
}

void object_free_string(ObjectString *string)
{
    free(string->characters);
    string->characters = NULL;
    string->length = 0;
    free(string);
    // object_clear(&string->object);
}
