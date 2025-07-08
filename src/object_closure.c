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
}

void ObjectClosure_free(ObjectClosure* closure) {
    free(closure);
    closure->function = NULL;
}