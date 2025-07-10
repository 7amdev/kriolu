#include "kriolu.h"

//
// ObjectUpvalue
// 

ObjectUpvalue* ObjectUpvalue_allocate(Object** object_head, Value* value_addresss) {
    ObjectUpvalue* upvalue = calloc(1, sizeof(ObjectUpvalue));
    assert(upvalue);
    ObjectUpvalue_init(upvalue, object_head, value_addresss);

    return upvalue;
}

void ObjectUpvalue_init(ObjectUpvalue* upvalue, Object** object_head, Value* value_address) {
    Object_init((Object*)upvalue, ObjectKind_Upvalue, object_head);
    assert(upvalue->object.kind == ObjectKind_Upvalue);
    upvalue->value_address = value_address;
}

void ObjectUpvalue_free(ObjectUpvalue* upvalue) {
    free(upvalue);
    upvalue->value_address = NULL;
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