// 2025.03.17
// Alfredo Monteiro
//
// HashTable:
//     Associates a set of Keys to a set of Values.
//     Each Key/Value pair is an Entry in the Table.
//     Tecnically, HashTable is a Dynamic Array with
//     very sophisticated policies on reading, writing, and
//     delete elements.
//     Constant time lookup.
//
// HashTable core:
//     Hash functions, Dynamic Resizing, and Collision Resolution,

#include "kriolu.h"

// This guarantees that there'll always be an empty entry
// in the entries array, which prevents inifinite loop
// when probing/search for a specific entry.
//
#define TABLE_MAX_LOAD 0.75

static Entry* hash_table_find_entry(Entry* entries, ObjectString* key, int capacity);
static Entry* hash_table_find_entry_by_key(Entry* entries, ObjectString* key, int capacity);
static void hash_table_adjust_capacity(HashTable* table, int capacity);
static Entry hash_table_make_tombstone();
static bool hash_table_is_an_empty_entry(Entry* entry);
static bool hash_table_is_a_tombstone_entry(Entry* entry);

void hash_table_init(HashTable* table)
{
    table->entries = NULL;
    table->count = 0;
    table->capacity = 0;
}

void hash_table_copy(HashTable* from, HashTable* to)
{
    for (int i = 0; i < from->capacity; i++)
    {
        Entry* entry = &from->entries[i];
        if (entry->key != NULL)
        {
            hash_table_set_value(to, entry->key, entry->value);
        }
    }
}

bool hash_table_set_value(HashTable* table, ObjectString* key, Value value)
{
    if (table->count + 1 > table->capacity * TABLE_MAX_LOAD)
    {
        int capacity = table->capacity < 8 ? 8 : 2 * table->capacity;
        hash_table_adjust_capacity(table, capacity);
    }

    // Entry *entry = hash_table_find_entry(table->entries, key, table->capacity);
    Entry* entry = hash_table_find_entry_by_key(table->entries, key, table->capacity);
    bool is_new_key = (entry->key == NULL);
    if (hash_table_is_an_empty_entry(entry))
        table->count += 1;

    entry->key = key;
    entry->value = value;

    return is_new_key;
}

bool hash_table_get_value(HashTable* table, ObjectString* key, Value* value_out)
{
    if (table->count == 0)
        return false;

    // Entry *entry = hash_table_find_entry(table->entries, key, table->capacity);
    Entry* entry = hash_table_find_entry_by_key(table->entries, key, table->capacity);
    if (entry->key == NULL)
        return false;

    *value_out = entry->value;
    return true;
}

ObjectString* hash_table_get_key(HashTable* table, String string, uint32_t hash)
{
    if (table->count == 0)
        return NULL;

    uint32_t index = hash % table->capacity;
    for (;;)
    {
        Entry* entry = &table->entries[index];
        if (hash_table_is_an_empty_entry(entry))
            return NULL;

        if (entry->key->length == string.length &&
            entry->key->hash == hash &&
            memcmp(entry->key->characters, string.characters, entry->key->length) == 0)
        {
            return entry->key;
        }

        index = (index + 1) % table->capacity;
    }
}

bool hash_table_delete(HashTable* table, ObjectString* key)
{
    if (table->count == 0)
        return false;

    // Entry *entry = hash_table_find_entry(table->entries, key, table->capacity);
    Entry* entry = hash_table_find_entry_by_key(table->entries, key, table->capacity);

    if (hash_table_is_a_tombstone_entry(entry) ||
        hash_table_is_an_empty_entry(entry))
        return false;

    *entry = hash_table_make_tombstone();
    return true;
}

void hash_table_free(HashTable* table)
{
    free(table->entries);
    free(table);
    hash_table_init(table);
}

static Entry hash_table_make_tombstone()
{
    Entry tombstone;
    tombstone.key = NULL;
    tombstone.value = value_make_boolean(true);

    return tombstone;
}

static bool hash_table_is_an_empty_entry(Entry* entry)
{
    return (entry->key == NULL && value_is_nil(entry->value));
}

static bool hash_table_is_a_tombstone_entry(Entry* entry)
{
    return (
        entry->key == NULL &&
        value_is_boolean(entry->value) &&
        value_as_boolean(entry->value) == true);
}

static bool hash_table_is_hash_collision(ObjectString* a, ObjectString* b)
{
    return (a->length == b->length &&
            a->hash == b->hash &&
            memcmp(a->characters, b->characters, a->length) != 0);
}

static bool hash_table_is_key_collision(Entry* entry, ObjectString* key)
{
    return (
        entry->key->length != key->length ||
        entry->key->hash != key->hash ||
        memcmp(entry->key->characters, key->characters, entry->key->length) != 0);
}

// Probing is a sequence of non-empty entries terminated by an empty entry.
// We know we’ve reached the end of a sequence and that the
// when we hit an empty bucket.
//
// Return: entry-found | entry-empty | entry-tombstone
//
static Entry* hash_table_probing(Entry* entries, ObjectString* key, int capacity)
{
    uint32_t index = key->hash % capacity;
    Entry* entry = &entries[index];
    Entry* tombstone = NULL;

    for (;;)
    {
        if (hash_table_is_an_empty_entry(entry))
            return tombstone != NULL ? tombstone : entry;

        if (hash_table_is_a_tombstone_entry(entry))
            tombstone = entry;

        // This is possible because string deduplication is implemented, otherwise
        // key characters must be equal.
        //
        if (entry->key == key)
            return entry;

        index = (index + 1) % capacity;
    }
}

static Entry* hash_table_find_entry_by_key(Entry* entries, ObjectString* key, int capacity)
{
    uint32_t index = key->hash % capacity;
    Entry* entry = &entries[index];

    if (entry->key == key)
        return entry;

    if (hash_table_is_a_tombstone_entry(entry))
        return entry;

    if (hash_table_is_an_empty_entry(entry))
        return entry;

    if (hash_table_is_key_collision(entry, key))
        return hash_table_probing(entries, key, capacity);

    assert(false && "Error: invalid index. Provide a valid Key or check if entries capacity is valid!");
    return NULL;
}

static Entry* hash_table_find_entry(Entry* entries, ObjectString* key, int capacity)
{
    uint32_t index = key->hash % capacity;
    Entry* tombstone = NULL;
    for (;;)
    {
        Entry* entry = &entries[index];
        if (hash_table_is_an_empty_entry(entry))
            return tombstone != NULL ? tombstone : entry;

        if (entry->key == key)
            return entry;

        if (hash_table_is_a_tombstone_entry(entry))
            tombstone = entry;

        index = (index + 1) % capacity;
    }
}

static void hash_table_adjust_capacity(HashTable* table, int capacity)
{
    // Allocate an empty array of Entry(Key/Value Pair)
    //
    Entry* entries = (Entry*)realloc(NULL, sizeof(Entry) * capacity);
    assert(entries);
    for (int i = 0; i < capacity; i++)
    {
        entries[i].key = NULL;
        entries[i].value = value_make_nil();
    }

    // Copy entry values from "table->entries" to the new allocated "entries" variable.
    // Since the hash-table capacity changed, the individual entry will
    // be indexed in a different bucket, therefore the need to call
    // hash_table_find_entry() to recalculate the new position.
    //
    table->count = 0;
    for (int i = 0; i < table->capacity; i++)
    {
        Entry* entry = &table->entries[i];
        if (entry->key == NULL)
            continue;

        // Entry *dest = hash_table_find_entry(entries, entry->key, capacity);
        Entry* dest = hash_table_find_entry_by_key(entries, entry->key, capacity);
        dest->key = entry->key;
        dest->value = entry->value;
        table->count += 1;
    }

    free(table->entries);
    table->entries = entries;
    table->capacity = capacity;
}
