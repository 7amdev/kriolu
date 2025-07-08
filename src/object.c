#include "kriolu.h"

Object* Object_allocate(ObjectKind kind, size_t size, Object** object_head)
{
    Object* object = (Object*)calloc(1, sizeof(Object));
    assert(object);

    Object_init(object, kind, object_head);
    return object;
}

void Object_init(Object* object, ObjectKind kind, Object** object_head)
{
    object->kind = kind;

    // Linked List
    //
    if (object_head != NULL) {
        object->next = *object_head;
        *object_head = object;
    }
}

void Object_clear(Object* object)
{
    object->kind = ObjectKind_Invalid;
}

void Object_print_function(ObjectFunction* function) {
    if (function->name == NULL) {
        printf("<script>");
        return;
    }

    printf("<fn %s>", function->name->characters);
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
        Object_print_function(function);
        // if (function->name == NULL) {
        //     printf("<script>");
        //     return;
        // }
        // printf("<fn %s>", function->name->characters);
    } break;
    case ObjectKind_Function_Native: {
        printf("<fn native>");
    } break;
    case ObjectKind_Closure: {
        ObjectFunction* function = ((ObjectClosure*)object)->function;
        Object_print_function(function);
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
    case ObjectKind_Function_Native: {
        ObjectFunctionNative_free((ObjectFunctionNative*)object);
    } break;
    }
}
