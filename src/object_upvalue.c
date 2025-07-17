#include "kriolu.h"

//
// ObjectUpvalue
// 

ObjectUpvalue* ObjectUpvalue_allocate(Object** object_head, Value* value_addresss, ObjectUpvalue* next) {
    ObjectUpvalue* upvalue = calloc(1, sizeof(ObjectUpvalue));
    assert(upvalue);
    ObjectUpvalue_init(upvalue, object_head, value_addresss, next);

    return upvalue;
}

void ObjectUpvalue_init(ObjectUpvalue* upvalue, Object** object_head, Value* value_address, ObjectUpvalue* next) {
    Object_init((Object*)upvalue, ObjectKind_Upvalue, object_head);
    assert(upvalue->object.kind == ObjectKind_Upvalue);
    upvalue->value_address = value_address;
    upvalue->value = value_make_nil(); // (Value) { 0 };
    upvalue->next = next;
}

// Upvalue->value_address is stored in descendent order, because its
// capturing memory-address from a stack data structure.
// 
UpvalueFindResult ObjectUpvalue_find(ObjectUpvalue* item_current, Value* value_address) {
    ObjectUpvalue* item_previous = NULL;
    while (item_current != NULL && item_current->value_address > value_address) {
        item_previous = item_current;
        item_current = item_current->next;
    }

    return (UpvalueFindResult) { item_previous, item_current };
}

void ObjectUpvalue_free(ObjectUpvalue* upvalue) {
    free(upvalue);
    upvalue->value_address = NULL;
    upvalue->next = NULL;
}

//
// ArrayObjectUpvalue
// 

ArrayObjectUpvalue* ArrayObjectUpvalue_allocate() {
    ArrayObjectUpvalue* upvalues = calloc(1, sizeof(ArrayObjectUpvalue));
    assert(upvalues);

    return upvalues;
}

void ArrayObjectUpvalue_init(ArrayObjectUpvalue* upvalues, int item_count) {
    ObjectUpvalue** items = malloc(item_count * sizeof(ObjectUpvalue*));
    assert(items);
    for (int i = 0; i < item_count; i++) items[i] = NULL;

    upvalues->items = items;
    upvalues->count = item_count;
}

void ArrayObjectUpvalue_free(ArrayObjectUpvalue* upvalues) {
    // TODO:
    // loop upvalues->count -> upvalues->items[i] = NULL;
    free(upvalues->items);
    upvalues->count = 0;
    upvalues->items = NULL;
    // free(upvalues);
}