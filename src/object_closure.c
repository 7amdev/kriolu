#include "kriolu.h"

ObjectClosure* ObjectClosure_allocate(ObjectFunction* function, Object** object_head) {
    ObjectClosure* closure = calloc(1, sizeof(ObjectClosure));
    assert(closure);

    ObjectClosure_init(closure, function, object_head);
    return closure;
}

void ObjectClosure_init(ObjectClosure* closure, ObjectFunction* function, Object** object_head) {
    Object_init((Object*)closure, ObjectKind_Closure, object_head);
    assert(closure->object.kind == ObjectKind_Closure);

    closure->function = function;
    ArrayObjectValue_init(&closure->heap_values, function->variable_dependencies_count);
}

void ObjectClosure_free(ObjectClosure* closure) {
    ArrayObjectValue_free(&closure->heap_values);
    free(closure);
    closure->function = NULL;
}