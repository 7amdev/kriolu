#include "kriolu.h"

ObjectString* ObjectString_allocate(char* characters, int length, uint32_t hash) {
    ObjectString* object_string = calloc(1, sizeof(ObjectString));
    assert(object_string);
    ObjectString_init(object_string, characters, length, hash);

    return object_string;
}

ObjectString* ObjectString_is_interned(HashTable* table, String string) {
    uint32_t hash = string_hash(string);
    ObjectString* object_string = hash_table_get_key(&g_vm.string_database, string, hash);

    return object_string;
}

ObjectString* ObjectString_allocate_if_not_interned(HashTable* table, const char* characters, int length) {
    assert(characters);

    String source_string = string_make(characters, length);
    uint32_t hash = string_hash(source_string);
    ObjectString* string = hash_table_get_key(&g_vm.string_database, source_string, hash);
    if (string == NULL) {
        source_string = string_copy(characters, length);
        hash = string_hash(source_string);
        string = ObjectString_AllocateAndIntern(source_string.characters, source_string.length, hash);
    }

    return string;
}

ObjectString* ObjectString_allocate_and_intern(HashTable* table, char* characters, int length, uint32_t hash) {
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
void ObjectString_free(ObjectString* string) {
    free(string->characters);
    free(string);
    string->characters = NULL;
    string->length = 0;
}
