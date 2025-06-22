#include "kriolu.h"

ObjectFunctionNative* ObjectFunctionNative_allocate(FunctionNative* function, Object** object_head) {
    ObjectFunctionNative* native = calloc(1, sizeof(ObjectFunctionNative));
    assert(native);
    ObjectFunctionNative_init(native, function, object_head);

    return native;
}

void ObjectFunctionNative_init(ObjectFunctionNative* native, FunctionNative* function, Object** object_head) {
    Object_init((Object*)native, ObjectKind_Function_Native, object_head);
    assert(native->object.kind == ObjectKind_Function_Native);
    native->function = function;
}


void ObjectFunctionNative_free(ObjectFunctionNative* native) {
    free(native);
    native->function = NULL;
    native->object = (Object){ 0 };
}