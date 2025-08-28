#include "kriolu.h"

// TODO: 
// [] Remove all the malloc, ralloc, and calloc by Mamory_allocate
// [] Review Object_free if its covering all the object's type
// [] Implement a generic Stack API
// [] Implement Generic Dynamic Array
// [] Implement garbage collector
// [x] Review Garbage collection operation order
// [x] Organize the API interface

///Mark Phase process: 
// 1. Start off with all objects white
// 2. Find all the roots and merk them gray
// 3. Repeat as long as there are still gray objects:
//    3.1 Pick gray object. Turn any white objects that the object mentions to gray
//    3.2 Mark the original object Black

size_t *bytes_total;
size_t  bytesTotal;
size_t  bytes_threshold = Megabytes(2); 

static void Memory_collect_garbage();
static void Memory_mark_roots();
static void Memory_mark_array();
static void Memory_trace_references();
static void Memory_blacken_object(Object* object);
static void Memory_sweep();
static void Memory_free_object(Object* object);

void* Memory_allocate(void* pointer, size_t old_size, size_t new_size) {
    bool is_allocation   = (new_size > old_size);
    bool is_deallocation = (new_size == 0);

    if (bytes_total == NULL) Memory_register(NULL); 

    *bytes_total += new_size - old_size;

    if (is_allocation) {
#ifdef DEBUG_GC_STRESS
        Memory_collect_garbage();
#endif // DEBUG_GC_STRESS

        if (*bytes_total > bytes_threshold) {
            Memory_collect_garbage();
        }
    }

    if (is_deallocation) {
        free(pointer);
        return NULL;
    }

    // Expand or Shrink the Array
    // 
    void* result = realloc(pointer, new_size);
    if (result == NULL) exit(1);

    return result;
}

void Memory_register(size_t* bytes_total_source) {
    if (bytes_total_source == NULL) {
        bytes_total = &bytesTotal;
        return;
    }

    bytes_total = bytes_total_source;
}

void Memory_mark_object(Object* object){
}

void Memory_mark_value(Value value) {
}

void Memory_free_objects() {
}

//
// Private
//

static void Memory_collect_garbage() {
}

static void Memory_mark_roots() {
}

static void Memory_mark_array() {
}

static void Memory_trace_references() {
}

static void Memory_blacken_object(Object* object) {
}

static void Memory_sweep() {
}

static void Memory_free_object(Object* object) {
}