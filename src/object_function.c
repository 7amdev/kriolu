#include "kriolu.h"

ObjectFunction* ObjectFunction_allocate() {
    ObjectFunction* function = calloc(1, sizeof(ObjectFunction));
    assert(function);
    ObjectFunction_init(function, NULL, &function->bytecode, 0);

    return function;
}

void ObjectFunction_init(ObjectFunction* function, ObjectString* name, Bytecode* bytecode, int arity) {
    Object_init((Object*)function, ObjectKind_Function);
    assert(function->object.kind == ObjectKind_Function);
    function->arity = arity;
    function->name = name;
    Bytecode_init(bytecode);
}

void ObjectFunction_free(ObjectFunction* function) {
    Bytecode_free(&function->bytecode);
    free(function);
    function->arity = 0;
    function->bytecode = (Bytecode){ 0 };
    function->name = NULL;
    function->object = (Object){ 0 };

    // Will be handled by the Garbage Collector
    // ObjectString_free(function->name); 
}