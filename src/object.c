#include "kriolu.h"

Object* Object_allocate(ObjectKind kind, size_t size, Object** object_head) {
    Object* object = Memory_Allocate(Object, 1);
    assert(object);

    Object_init(object, kind, object_head);
    return object;
}

void Object_init(Object* object, ObjectKind kind, Object** object_head) {
    object->kind = kind;
    if (object_head != NULL) LinkedList_push(*object_head, object);
}

ObjectString* ObjectString_allocate(AllocateParams params) {
    ObjectString* object_st = NULL;

    // .Check if it doesn't already exist
    if (params.task & AllocateTask_Check_If_Interned)
        object_st = hash_table_get_key(
            params.table,
            params.string,
            params.hash
        );

    if (object_st == NULL) {
        // .Allocate
        object_st = Memory_Allocate(ObjectString, 1);
        assert(object_st);

        // .Copy String
        String string_copy = params.string;
        if (params.task & AllocateTask_Copy_String)
            string_copy = string_copy_from_other(params.string);

        // .Initialize
        if (params.task & AllocateTask_Initialize) {
            // TODO: assert mandatory params for this section
            //
            // ((Object*)object_st)->kind = ObjectKind_String;
            // LinkedList_push(*params.first, (Object*)object_st);
            Object_init((Object*)object_st, ObjectKind_String, params.first);
            object_st->characters = string_copy.characters;
            object_st->length = string_copy.length;
            object_st->hash = params.hash;
        }

        // .Intern
        // TODO: assert mandatory params for this section
        if (params.task & AllocateTask_Intern)
            hash_table_set_value(params.table, object_st, value_make_nil());
    }

    return object_st;
}

ObjectFunction* ObjectFunction_allocate(ObjectString* function_name, Object** object_head) {
    ObjectFunction* object_fn = Memory_Allocate(ObjectFunction, 1);
    assert(object_fn);

    // ((Object*)object_fn)->kind = ObjectKind_Function;
    // LinkedList_push(parser->objects, (Object*)object_fn);
    Object_init((Object*)object_fn, ObjectKind_Function, object_head);
    object_fn->arity = 0;
    object_fn->name = function_name;
    object_fn->variable_dependencies_count = 0;
    Bytecode_init(&object_fn->bytecode);

    return object_fn;
}

ObjectValue* ObjectValue_allocate(Object** object_head, Value* value_address) {
    ObjectValue* object_value = Memory_Allocate(ObjectValue, 1);
    assert(object_value);

    // ((Object*)object_value)->kind = ObjectKind_Heap_Value;
    // LinkedList_push(*object_head, (Object*)object_value);
    Object_init((Object*)object_value, ObjectKind_Heap_Value, object_head);
    object_value->value_address = value_address;
    object_value->value = value_make_nil(); 

    return object_value;
}

ObjectClosure* ObjectClosure_allocate(ObjectFunction* function, Object** object_head) {
    int item_count = function->variable_dependencies_count;
    ObjectValue** items = NULL;
    if (item_count > 0) {
       items = Memory_Allocate(ObjectValue*, item_count);
       assert(items);
       for (int i = 0; i < item_count; i++) items[i] = NULL;
    }
    
    ObjectClosure* closure = Memory_Allocate(ObjectClosure, 1);
    assert(closure);

    // ((Object*)closure)->kind =  ObjectKind_Closure;
    // LinkedList_push(*object_head, (Object*)closure);
    Object_init((Object*)closure, ObjectKind_Closure, object_head);
    closure->function = function;
    closure->heap_values.items = items;
    closure->heap_values.count = item_count;

    return closure;
}

ObjectFunctionNative* ObjectFunctionNative_allocate(FunctionNative* function, Object** object_head, int arity) {
    ObjectFunctionNative* native = Memory_Allocate(ObjectFunctionNative, 1);
    assert(native);
    
    // ((Object*)native)->kind = ObjectKind_Function_Native;
    // LinkedList_push(*object_head, (Object*)native);
    Object_init((Object*)native, ObjectKind_Function_Native, object_head);
    native->function = function;
    native->arity = arity;

    return native;
}

void Object_print_function(ObjectFunction* function) {
    if (function->name == NULL) {
        printf("<script>");
        return;
    }

    printf("<fn %s>", function->name->characters);
}

void Object_print(Object* object) {
    switch (object->kind)
    {
    case ObjectKind_String: {
        ObjectString* string = ((ObjectString*)object);
        printf("%.10s", string->characters);
        if (string->length > 10)
            printf("...");

    } break;
    case ObjectKind_Function: {
        ObjectFunction* function = (ObjectFunction*)object;
        Object_print_function(function);
    } break;
    case ObjectKind_Function_Native: {
        printf("<fn native>");
    } break;
    case ObjectKind_Closure: {
        ObjectFunction* function = ((ObjectClosure*)object)->function;
        Object_print_function(function);
    } break;
    case ObjectKind_Heap_Value: {
        printf("Heap Value");
    } break;
    }
}

void ObjectString_free(ObjectString* object_st) {
    Memory_Free(char*, object_st->characters);
    Memory_Free(ObjectString, object_st);
    object_st = NULL;
}

void ObjectFunction_free(ObjectFunction* function) {
    ObjectString_free(function->name);
    Memory_Free(ObjectFunction, function);
    function = NULL;
}

void Object_free(Object* object) {
    switch (object->kind)
    {
    case ObjectKind_String: {
        ObjectString* object_st = (ObjectString*)object;
        Memory_FreeArray(char, object_st->characters, object_st->length + 1);
        ObjectString_free(object_st);
    } break;
    case ObjectKind_Function: {
        ObjectFunction* object_fn = (ObjectFunction*)object;
        Bytecode_free(&object_fn->bytecode);
        ObjectFunction_free(object_fn);
    } break;
    case ObjectKind_Heap_Value : {
        Memory_Free(ObjectValue, object);
        object = NULL;
    } break;
    case ObjectKind_Closure : {
        // NOTE: Closure doesn't own ObjectValue nor ObjectFunction. Other Closures might be using the same 
        //       ObjectValue or ObjectFunction instance.
        //
        ObjectClosure* object_cl = (ObjectClosure*)object;
        Memory_FreeArray(ObjectValue*, object_cl->heap_values.items, object_cl->heap_values.count);
        Memory_Free(ObjectClosure, (ObjectClosure*)object);
        object = NULL;
    } break;
    case ObjectKind_Function_Native: {
        Memory_Free(ObjectFunctionNative, object);
        object = NULL;
    } break;
    }
}
