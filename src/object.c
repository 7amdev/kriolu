#include "kriolu.h"

Object* Object_allocate(ObjectKind kind, size_t size)
{
    Object* object = (Object*)calloc(1, sizeof(Object));
    assert(object);

    Object_init(object, kind);
    return object;
}

void Object_init(Object* object, ObjectKind kind)
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

void Object_clear(Object* object)
{
    object->kind = ObjectKind_Invalid;
}

void Object_print(Object* object)
{
    switch (object->kind)
    {
    case ObjectKind_String: {
        ObjectString* string = ((ObjectString*)object);
        printf("%.10s", string->characters);
        if (string->length > 10)
            printf("...");

    } break;
    case ObjectKind_Function: {
        ObjectFunction* function = (ObjectFunction*)object;
        if (function->name == NULL) {
            printf("<script>");
            return;
        }
        printf("<fn %s>", function->name->characters);
    } break;
    }
}

void Object_free(Object* object)
{
    switch (object->kind)
    {
    case ObjectKind_String: {
        // TODO: update virtual machine's bytes allocated variable
        ObjectString_free((ObjectString*)object);
    } break;
    case ObjectKind_Function: {
        ObjectFunction_free((ObjectFunction*)object);
    } break;
    }
}
