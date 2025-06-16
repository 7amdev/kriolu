#include "kriolu.h"

ObjectString* ObjectString_allocate(char* characters, int length, uint32_t hash, Object** object_head) {
    ObjectString* object_string = calloc(1, sizeof(ObjectString));
    assert(object_string);
    ObjectString_init(object_string, characters, length, hash, object_head);

    return object_string;
}

// TODO: Convert this function to a macro
ObjectString* ObjectString_is_interned(HashTable* table, String string) {
    uint32_t hash = string_hash(string);
    ObjectString* object_string = hash_table_get_key(table, ObjectString_from_string(string));

    return object_string;
}

ObjectString* ObjectString_allocate_if_not_interned(HashTable* table, const char* characters, int length, Object** object_head) {
    assert(characters);

    String source_string = string_make(characters, length);
    // uint32_t hash = string_hash(source_string);
    // ObjectString* string = hash_table_get_key(table, object_string);

    // ObjectString object_string = ObjectString_from_string(source_string);
    ObjectString* string = hash_table_get_key(table, ObjectString_from_string(source_string));
    if (string == NULL) {
        source_string = string_copy(characters, length);
        uint32_t hash = string_hash(source_string);
        string = ObjectString_allocate_and_intern(table, source_string.characters, source_string.length, hash, object_head);
    }

    return string;
}

ObjectString* ObjectString_allocate_and_intern(HashTable* table, char* characters, int length, uint32_t hash, Object** object_head) {
    ObjectString* string = ObjectString_allocate(characters, length, hash, object_head);
    assert(string);
    hash_table_set_value(table, string, value_make_nil());
    return string;
}

void ObjectString_init(ObjectString* object_string, char* characters, int length, uint32_t hash, Object** object_head) {
    Object_init((Object*)object_string, ObjectKind_String, object_head);
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

ObjectString ObjectString_from_string(String string) {
    ObjectString object_string = { 0 };
    uint32_t hash = string_hash(string);

    object_string.object.kind = ObjectKind_String;
    object_string.characters = string.characters;
    object_string.length = string.length;
    object_string.hash = hash;

    return object_string;
}

void ObjectString_free(ObjectString* string) {
    free(string->characters);
    free(string);
    string->characters = NULL;
    string->length = 0;
}
