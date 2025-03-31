#include "kriolu.h"

Object* object_allocate(ObjectKind kind, size_t size)
{
    Object* object = (Object*)calloc(1, sizeof(Object));
    assert(object);

    object_init(object, kind);
    return object;
}

void object_init(Object* object, ObjectKind kind)
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

void object_clear(Object* object)
{
    object->kind = ObjectKind_Invalid;
}

void object_print(Object* object)
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

void object_free(Object* object)
{
    switch (object->kind)
    {
    case ObjectKind_String:
    {
        // TODO: update virtual machine's bytes allocated variable
        object_free_string((ObjectString*)object);
        break;
    }
    }
}

ObjectString* object_allocate_string(char* characters, int length, uint32_t hash)
{
    ObjectString* string = calloc(1, sizeof(ObjectString));
    assert(string);

    object_init((Object*)string, ObjectKind_String);
    assert(string->object.kind == ObjectKind_String);
    string->characters = characters;
    string->hash = hash;
    string->length = length;

    return string;
}

ObjectString* object_allocate_string_if_not_interned(HashTable* table, const char* characters, int length) {
    assert(characters);

    String source_string = string_make(characters, length);
    uint32_t hash = string_hash(source_string);
    ObjectString* string = hash_table_get_key(&g_vm.strings, source_string, hash);
    if (string == NULL) {
        source_string = string_make_and_copy_characters(characters, length);
        hash = string_hash(source_string);
        string = object_create_and_intern_string(source_string.characters, source_string.length, hash);
    }

    return string;
}

ObjectString* object_allocate_and_intern_string(HashTable* table, char* characters, int length, uint32_t hash)
{
    ObjectString* string = object_allocate_string(characters, length, hash);
    assert(string);
    hash_table_set_value(table, string, value_make_nil());
    return string;
}


String object_to_string(ObjectString* object_string) {
    String string = { 0 };
    string.characters = object_string->characters;
    string.length = object_string->length;

    return string;

}
void object_free_string(ObjectString* string)
{
    free(string->characters);
    free(string);
    string->characters = NULL;
    string->length = 0;
}
