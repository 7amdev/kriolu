#include "kriolu.h"

///TODO: 
// []  Implement garbage collector
// [x] HashTable free reroute to Memory_Free
// [x] Object_free reroute to Memory_Free
// [x] Bytecode free reroute to Memory_Free
// [x] Review Object_free if its covering all the object's type
// [x] Remove all the malloc, ralloc, and calloc by Mamory_allocate
// [x] Review Garbage collection operation order
// [x] Organize the API interface

///Mark Phase process: 
// 1. Start off with all objects white
// 2. Find all the roots and merk them gray
// 3. Repeat as long as there are still gray objects:
//    3.1 Pick gray object. Turn any white objects that the object mentions to gray
//    3.2 Mark the original object Black

#define Memory_Threshold_Growth_Factor 2

typedef struct {
    DynamicArrayHeader;
    Object** items;
} DynamincArrayGray;

size_t             bytes_total      = 0;
size_t             bytes_threshold  = Megabytes(2); 
size_t            *M_bytes_total    = &bytes_total;
Parser            *M_parser         = NULL;
VirtualMachine    *M_vm             = NULL;
DynamincArrayGray  M_Greys          = {0};

static void Memory_collect_garbage();
static void Memory_mark_roots();
static void Memory_mark_object_black(Object *object);       // TODO: delete function
static void Memory_mark_values_gray(ArrayValue *values);
static void Memory_mark_hashtable_gray(HashTable* table);
static void Memory_blacken_objects();
static void Memory_sweep_string_database();
static void Memory_sweep();
static void Memory_free_object(Object* object);

void Memory_register(size_t* bytes_total_source, VirtualMachine *vm, Parser *parser) {
    if (M_bytes_total == NULL) 
    if (bytes_total_source != NULL) 
        M_bytes_total = bytes_total_source;; 
    
    if (M_vm == NULL && vm)
        M_vm = vm;

    if (M_parser == NULL && parser) 
        M_parser = parser;
}

void* Memory_allocate(void* pointer, size_t old_size, size_t new_size) {
    bool is_allocation   = (new_size > old_size);
    bool is_deallocation = (new_size == 0);

    *M_bytes_total += new_size - old_size;

    if (is_allocation) 
    {

#ifdef DEBUG_GC_STRESS
        Memory_collect_garbage();
#endif // DEBUG_GC_STRESS

        if (*M_bytes_total > bytes_threshold) {
            Memory_collect_garbage();
        }
    }

    if (is_deallocation) 
    {
        free(pointer);
        return NULL;
    }

    // Expand or Shrink the Array
    // 
    void* result = realloc(pointer, new_size);
    if (result == NULL) exit(1);

    return result;
}

void Memory_free_objects() {
}

//
// Private
//


static void Memory_collect_garbage() {
#ifdef DEBUG_GC_TRACE
    printf("-- Garbage Collector::Begin\n");
    size_t size_before = *M_bytes_total;
#endif // DEBUG_GC_TRACE

    Memory_mark_roots();        // NOTE: Mark objects as 'Gray'.
    Memory_blacken_objects();   //       Mark objects as 'Black'
    Memory_sweep_string_database();
    Memory_sweep();

    bytes_threshold = *M_bytes_total * Memory_Threshold_Growth_Factor;

#ifdef DEBUG_GC_TRACE
    printf(
        "--   Collected '%zu' bytes from (from '%zu' to '%zu') next at '%zu'\n", 
        size_before - *M_bytes_total, 
        size_before, 
        *M_bytes_total, 
        bytes_threshold
    );
    printf("-- Garbage Collector::End\n");
#endif // DEBUG_GC_TRACE
}

static void Memory_mark_roots() {
    // Mark Vm's Stack Values
    //
    Value value = {0};
    for (int i = 0; i < StackPointer_count(&M_vm->stack_value); i++) {
        StackPointer_peek(&M_vm->stack_value, &value, i);
        Memory_mark_value_gray(value);
    }

    // Mark Globals - HashTable
    //
    Memory_mark_hashtable_gray(&M_vm->global_database);

    // TODO: delete code bellow. Its been replaced by 
    //       the function 'Memory_mark_hashtable_gray'
    //
    // for (int i = 0; i < M_vm->global_database.capacity; i++) {
    //     Entry *entry = &M_vm->global_database.items[i];
    //     Memory_mark_object_gray((Object*)entry->key);
    //     Memory_mark_value_gray(entry->value);
    // }

    // Mark FunctionCalls
    //
    for (int i = 0; i < M_vm->function_calls.top; i++) {
        Memory_mark_object_gray((Object*)M_vm->function_calls.items[i].closure);
    }

    // Mark HeapValues
    //
    LinkedList_foreach(ObjectValue, M_vm->heap_values, object_value) {
        Memory_mark_object_gray((Object*)object_value.curr);
    }

    // Mark Parser's functions chains
    // 
    if (M_parser) {
        LinkedList_foreach(Function, M_parser->function, function) {
            Memory_mark_object_gray((Object*)function.curr->object);
        }
    }
}

static void Memory_mark_object_gray(Object* object) {
    if (object == NULL)    return;
    if (object->is_marked) return;

    object->is_marked = true;
    DynamicArray_push(&M_Greys, object);

#ifdef DEBUG_GC_TRACE
    printf("--   '%p' mark ", (void*)object);
    Object_print(object);
    printf("\n");
#endif // DEBUG_GC_TRACE
}

static void Memory_mark_value_gray(Value value) {
    if (value_is_object(value)) 
        Memory_mark_object_gray(value_as_object(value));
}

static void Memory_mark_hashtable_gray(HashTable* table) {
    for (int i = 0; i < table->capacity; i++) {
        Entry *entry = &table->items[i];
        Memory_mark_object_gray((Object*)entry->key);
        Memory_mark_value_gray(entry->value);
    }
}

static void Memory_blacken_objects() {
//  A Black Object is any object whose 'is_maked' field 
//  is set to 'true' or 'Gray' and that is no longer in 'Gray' Stack.

    for (;;) {
        if (M_Greys.count == 0) break;

        Object* object = NULL;
        DynamicArray_pop(&M_Greys, &object);
        // if (object) Memory_mark_object_black(object);
        // OR
        // 
        if (object == NULL) continue;
        switch (object->kind)
        {
        case ObjectKind_Closure: {
            ObjectClosure *closure = (ObjectClosure*)object;
            Memory_mark_object_gray((Object*)closure->function);
            for (int i = 0; i < closure->heap_values.count; i++) {
                Memory_mark_object_gray((Object*)closure->heap_values.items[i]);
            }
        } break;
        case ObjectKind_Function: {
            ObjectFunction *function = (ObjectFunction*)object;
            Memory_mark_object_gray((Object*)function->name);
            Memory_mark_values_gray(&function->bytecode.values); 
        } break;
        case ObjectKind_Heap_Value: {
            Memory_mark_value_gray(((ObjectValue*)object)->value);
        } break;
        case ObjectKind_Class: {
            ObjectClass* klass = (ObjectClass*)object;
            Memory_mark_object_gray((Object*)klass->name);
        } break;
        case ObjectKind_Instance: {
            ObjectInstance* instance = (ObjectInstance*)object;
            Memory_mark_object_gray((Object*)instance->klass);
            Memory_mark_hashtable_gray(&instance->fields);
        } break;
        case ObjectKind_Function_Native: 
        case ObjectKind_String: 
            break;
        }

#ifdef DEBUG_GC_TRACE
    printf("--   '%p' blacken ", (void*)object);
    Object_print(object);
    printf("\n");
#endif // DEBUG_GC_TRACE
    }
}

static void Memory_sweep_string_database() {
    for (int i = 0; i < M_vm->string_database.capacity; i++) {
        Entry *entry = &M_vm->string_database.items[i];
        if (entry->key != NULL) 
        if (entry->key->object.is_marked == false) 
        {
            hash_table_delete(&M_vm->string_database, entry->key);
        }
    }
}

static void Memory_sweep() {
    LinkedList_foreach(Object, M_vm->objects, object) {
        if (object.curr->is_marked) {
            object.curr->is_marked = false;
            continue;
        }
        if (object.prev == NULL) M_vm->objects = object.curr->next;
        else object.prev->next = object.curr->next;
        
        Object_free(object.curr);
    }
    //
    // OR 
    //
    // Object *previous = NULL;
    // LinkedList_foreach(Object, M_vm->objects, object) {
    //     Object* current = object.curr;
    //     if (current->is_marked) {
    //         previous = current;
    //         continue;
    //     } 
    //
    //     if (previous == NULL) M_vm->objects = current->next;
    //     else previous->next = current->next;
    // }
}

// TODO: delete function
//
static void Memory_mark_object_black(Object *object) {
//  A Black Object is any object whose 'is_maked' field 
//  is set to 'true' and that is no longer in 'Gray' stack.

    switch (object->kind)
    {
    case ObjectKind_Closure: {
        ObjectClosure *closure = (ObjectClosure*)object;
        Memory_mark_object_gray((Object*)closure->function);
        for (int i = 0; i < closure->heap_values.count; i++) {
            Memory_mark_object_gray((Object*)closure->heap_values.items[i]);
        }
    } break;
    case ObjectKind_Function: {
        ObjectFunction *function = (ObjectFunction*)object;
        Memory_mark_object_gray((Object*)function->name);
        Memory_mark_values_gray(&function->bytecode.values); 
    } break;
    case ObjectKind_Heap_Value: {
        Memory_mark_value_gray(((ObjectValue*)object)->value);
    } break;
    case ObjectKind_Function_Native: 
    case ObjectKind_String: 
        break;
    }
}

static void Memory_mark_values_gray(ArrayValue *values) {
    for (int i = 0; i < values->count; i++) {
        Memory_mark_value_gray(values->items[i]);
    }
}

static void Memory_free_object(Object* object) {
}

void Memory_transaction_push(Value value) {
    stack_value_push(&M_vm->stack_value, value);
}

void Memory_transaction_pop() {
    stack_value_pop(&M_vm->stack_value);
}