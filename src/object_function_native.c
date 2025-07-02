#include "kriolu.h"

ObjectFunctionNative* ObjectFunctionNative_allocate(FunctionNative* function, Object** object_head, int arity) {
    ObjectFunctionNative* native = calloc(1, sizeof(ObjectFunctionNative));
    assert(native);
    ObjectFunctionNative_init(native, function, object_head, arity);

    return native;
}

void ObjectFunctionNative_init(ObjectFunctionNative* native, FunctionNative* function, Object** object_head, int arity) {
    Object_init((Object*)native, ObjectKind_Function_Native, object_head);
    assert(native->object.kind == ObjectKind_Function_Native);
    native->function = function;
    native->arity = arity;
}


void ObjectFunctionNative_free(ObjectFunctionNative* native) {
    free(native);
    native->function = NULL;
    native->object = (Object){ 0 };
    native->arity = 0;
}