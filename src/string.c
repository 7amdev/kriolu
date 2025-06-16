#include "kriolu.h"

String* string_allocate(const char* characters, int length)
{
    String* string = (String*)malloc(sizeof(String));
    assert(string);
    string_init(string, characters, length);
    return string;
}

void string_init(String* string, const char* characters, int length)
{
    string->characters = (char*)malloc(sizeof(char) * length + 1);
    assert(string->characters);
    memcpy(string->characters, characters, length);
    string->characters[length] = '\0';
    string->length = length;
}

String string_make(const char* characters, int length)
{
    String string = { 0 };
    string.characters = (char*)characters;
    string.length = length;

    return string;
}

String string_make_from_format(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    char buffer[8092];
    vsnprintf(buffer, 8092, format, args);
    va_end(args);

    String string = string_copy(buffer, strlen(buffer));

    return string;
}

String string_copy(const char* characters, int length)
{
    String string = { 0 };
    string.characters = (char*)malloc(sizeof(char) * length + 1);
    assert(string.characters);
    memcpy(string.characters, characters, length);
    string.characters[length] = '\0';
    string.length = length;

    return string;
}

String string_copy_from_other(String other)
{
    String string = { 0 };
    string.characters = (char*)malloc(sizeof(char) * other.length + 1);
    assert(string.characters);
    memcpy(string.characters, other.characters, other.length);
    string.characters[other.length] = '\0';
    string.length = other.length;

    return string;
}

String string_concatenate(String a, String b)
{
    String string = { 0 };
    string.length = a.length + b.length;
    string.characters = (char*)malloc(sizeof(char) * string.length + 1);
    assert(string.characters);
    memcpy(string.characters, a.characters, a.length);
    memcpy(string.characters + a.length, b.characters, b.length);
    string.characters[string.length] = '\0';

    return string;
}

uint32_t string_hash(String string)
{
    uint32_t hash = 2166136261u;
    for (int i = 0; i < string.length; i++)
    {
        hash ^= (uint8_t)string.characters[i]; // XOR
        hash *= 16777619;
    }

    return hash;
}

bool string_equal(String a, String b)
{
    if (a.length != b.length)
        return false;

    return memcmp(a.characters, b.characters, a.length) == 0;
}

void string_free(String* string)
{
    free(string->characters);
    string->characters = NULL;
    string->length = 0;
    free(string);
}
