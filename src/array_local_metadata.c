#include "kriolu.h"

static int ArrayLocalMetadata_find_index(ArrayLocalMetadata* local_metadata, uint8_t index, LocalLocation location);

void ArrayLocalMetadata_init(ArrayLocalMetadata* local_metadata, int count) {
    local_metadata->count = count;
}

int ArrayLocalMetadata_add(ArrayLocalMetadata* local_metadata, uint8_t index, LocalLocation location, int* variable_dependencies_count) {
    assert(local_metadata->count < UINT8_COUNT && "You've reached the limit of runtime objects.");

    int local_metadata_index = ArrayLocalMetadata_find_index(local_metadata, index, location);
    if (local_metadata_index != -1) return local_metadata_index;

    if (local_metadata->count == UINT8_COUNT) return 0; /// Maybe -2??

    LocalMetadata* new_local_metadata = &local_metadata->items[local_metadata->count];
    new_local_metadata->index = index;
    new_local_metadata->location = location;
    local_metadata->count += 1;

    // Update ObjectFunction field 'variable_dependencies_count'
    //
    if (variable_dependencies_count != NULL)
        *variable_dependencies_count = local_metadata->count;

    return local_metadata->count - 1;
}

static int ArrayLocalMetadata_find_index(ArrayLocalMetadata* local_metadata, uint8_t index, LocalLocation location) {
    for (int i = 0; i < local_metadata->count; i++) {
        if (local_metadata->items[i].index == index && local_metadata->items[i].location == location)
            return i;
    }

    return -1;
}