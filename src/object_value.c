#include "kriolu.h"

//
// ObjectValue
// 

ObjectValue* ObjectValue_allocate(Object** object_head, Value* value_addresss, ObjectValue* next) {
    ObjectValue* object_value = calloc(1, sizeof(ObjectValue));
    assert(object_value);
    ObjectValue_init(object_value, object_head, value_addresss, next);

    return object_value;
}

void ObjectValue_init(ObjectValue* object_value, Object** object_head, Value* value_address, ObjectValue* next) {
    Object_init((Object*)object_value, ObjectKind_Heap_Value, object_head);
    assert(object_value->object.kind == ObjectKind_Heap_Value);
    object_value->value_address = value_address;
    object_value->value = value_make_nil(); // (Value) { 0 };
    object_value->next = next;
}

// LocalMetadata->value_address is stored in descendent order, because its
// capturing memory-address from a stack data structure.
// 
ObjectValueFindResult ObjectValue_find(ObjectValue* item_current, Value* value_address) {
    ObjectValue* item_previous = NULL;
    while (item_current != NULL && item_current->value_address > value_address) {
        item_previous = item_current;
        item_current = item_current->next;
    }

    return (ObjectValueFindResult) { item_previous, item_current };
}

void ObjectValue_free(ObjectValue* object_value) {
    free(object_value);
    object_value->value_address = NULL;
    object_value->next = NULL;
}

//
// ArrayHeapValue
// 

ArrayHeapValue* ArrayHeapValue_allocate() {
    ArrayHeapValue* heap_values = calloc(1, sizeof(ArrayHeapValue));
    assert(heap_values);

    return heap_values;
}

void ArrayHeapValue_init(ArrayHeapValue* heap_values, int item_count) {
    ObjectValue** items = malloc(item_count * sizeof(ObjectValue*));
    assert(items);
    for (int i = 0; i < item_count; i++) items[i] = NULL;

    heap_values->items = items;
    heap_values->count = item_count;
}

void ArrayHeapValue_free(ArrayHeapValue* heap_values) {
    // TODO:
    // loop heap_values->count -> heap_values->items[i] = NULL;
    free(heap_values->items);
    heap_values->count = 0;
    heap_values->items = NULL;
    // free(heap_values);
}