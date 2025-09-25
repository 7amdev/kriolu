#include "kriolu.h"

#define Object_Allocate(Type, object_kind, object_head) \
    (Type*)Object_allocate(object_kind, sizeof(Type), object_head)

Object* Object_allocate(ObjectKind kind, size_t size, Object** object_head) {
    Object* object = (Object*) Memory_allocate(NULL, 0, size);
    assert(object);
    
    object->kind      = kind;
    object->is_marked = false;
    if (object_head != NULL) LinkedList_push(*object_head, object);

#ifdef DEBUG_GC_TRACE
    const char* object_kind = "Invalid object type!";
    if (object->kind >= 0) 
    if (object->kind < ObjectKind_Count)  
        object_kind = ObjectKind_text[object->kind];
        
    printf("--  %p allocate %zu byte(s) for '%s'\n", (void*)object, size, object_kind);
#endif // DEBUG_GC_TRACE

    return object;
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
        object_st = Object_Allocate(ObjectString, ObjectKind_String, params.first);
        assert(object_st);

        // .Copy String
        String string_copy = params.string;
        if (params.task & AllocateTask_Copy_String)
            string_copy = string_copy_from_other(params.string);

        // .Initialize
        if (params.task & AllocateTask_Initialize) {
            // TODO: assert mandatory params for this section
            //
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
    ObjectFunction* object_fn = Object_Allocate(ObjectFunction, ObjectKind_Function, object_head);
    assert(object_fn);

    object_fn->arity = 0;
    object_fn->name = function_name;
    object_fn->variable_dependencies_count = 0;
    Bytecode_init(&object_fn->bytecode);

    return object_fn;
}

ObjectValue* ObjectValue_allocate(Object** object_head, Value* value_address) {
    ObjectValue* object_value = Object_Allocate(ObjectValue, ObjectKind_Heap_Value, object_head);
    assert(object_value);

    object_value->value_address = value_address;
    object_value->value = value_make_nil(); 

    return object_value;
}

ObjectClosure* ObjectClosure_allocate(ObjectFunction* function, Object** object_head) {
    int item_count = function->variable_dependencies_count;
    ObjectValue** items = NULL;
    if (item_count > 0) {
       items = Memory_Allocate_Count(ObjectValue*, item_count);
       assert(items);
       for (int i = 0; i < item_count; i++) items[i] = NULL;
    }
    
    ObjectClosure* closure = Object_Allocate(ObjectClosure, ObjectKind_Closure, object_head);
    assert(closure);

    closure->function = function;
    closure->heap_values.items = items;
    closure->heap_values.count = item_count;

    return closure;
}

ObjectFunctionNative* ObjectFunctionNative_allocate(FunctionNative* function, Object** object_head, int arity) {
    ObjectFunctionNative* native = Object_Allocate(ObjectFunctionNative, ObjectKind_Function_Native, object_head);
    assert(native);
    
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
#ifdef DEBUG_GC_TRACE
    const char* object_kind = "Invalid object type!";
    if (object->kind < ObjectKind_Count)  
    if (object->kind >= 0) 
        object_kind = ObjectKind_text[object->kind];
        
    printf("--  %p free type '%s'\n", (void*)object, object_kind);
#endif // DEBUG_GC_TRACE

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
        ObjectString_free(object_fn->name);
        Memory_Free(ObjectFunction, object_fn);
        object_fn = NULL;
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
