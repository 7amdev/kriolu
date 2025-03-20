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

ObjectString *object_allocate_string(char *characters, int length, uint32_t hash)
{
    ObjectString *string = calloc(1, sizeof(ObjectString));
    assert(string);

    object_init((Object *)string, ObjectKind_String);
    assert(string->object.kind == ObjectKind_String);
    string->characters = characters;
    string->hash = hash;
    string->length = length;

    hash_table_set_value(&g_vm.strings, string, value_make_nil());

    return string;
}

void object_free_string(ObjectString *string)
{
    free(string->characters);
    free(string);
    string->characters = NULL;
    string->length = 0;
}
