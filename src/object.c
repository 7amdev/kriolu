#include "kriolu.h"

Object* Object_allocate(ObjectKind kind, size_t size)
{
    Object* object = (Object*)calloc(1, sizeof(Object));
    assert(object);

    Object_init(object, kind);
    return object;
}

Object* Object_allocate_string(char* characters, int length, uint32_t hash) {
    ObjectString* object_string = ObjectString_allocate(characters, length, hash);
    assert(object_string);
    return (Object*)object_string;
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
    case ObjectKind_String:
    {
        ObjectString* string = ((ObjectString*)object);
        printf("%.10s", string->characters);
        if (string->length > 10)
            printf("...");

        break;
    }
    }
}

void Object_free(Object* object)
{
    switch (object->kind)
    {
    case ObjectKind_String:
    {
        // TODO: update virtual machine's bytes allocated variable
        ObjectString_free((ObjectString*)object);
        break;
    }
    }
}

//
// ObjectString
//

// TODO: move object_string code to object_string.c file
//
ObjectString* ObjectString_allocate(char* characters, int length, uint32_t hash)
{
    ObjectString* object_string = calloc(1, sizeof(ObjectString));
    assert(object_string);
    ObjectString_init(object_string, characters, length, hash);

    return object_string;
}

ObjectString* ObjectString_allocate_if_not_interned(HashTable* table, const char* characters, int length) {
    assert(characters);

    String source_string = string_make(characters, length);
    uint32_t hash = string_hash(source_string);
    ObjectString* string = hash_table_get_key(&g_vm.string_database, source_string, hash);
    if (string == NULL) {
        source_string = string_make_and_copy_characters(characters, length);
        hash = string_hash(source_string);
        string = ObjectString_AllocateAndIntern(source_string.characters, source_string.length, hash);
    }

    return string;
}

ObjectString* ObjectString_allocate_and_intern(HashTable* table, char* characters, int length, uint32_t hash)
{
    ObjectString* string = ObjectString_allocate(characters, length, hash);
    assert(string);
    hash_table_set_value(table, string, value_make_nil());
    return string;
}

void ObjectString_init(ObjectString* object_string, char* characters, int length, uint32_t hash) {
    Object_init((Object*)object_string, ObjectKind_String);
    assert(object_string->object.kind == ObjectKind_String);
    object_string->characters = characters;
    object_string->length = length;
    object_string->hash = hash;
}

String ObjectString_to_string(ObjectString* object_string) {
    String string = { 0 };
    string.characters = object_string->characters;
    string.length = object_string->length;

    return string;

}
void ObjectString_free(ObjectString* string)
{
    free(string->characters);
    free(string);
    string->characters = NULL;
    string->length = 0;
}
