#include "kriolu.h"

Object* Object_allocate(ObjectKind kind, size_t size, Object** object_head) {
    Object* object = (Object*)calloc(1, sizeof(Object));
    assert(object);

    Object_init(object, kind, object_head);
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
        object_st = calloc(1, sizeof(ObjectString));
        assert(object_st);

        // .Copy String
        String string_copy = params.string;
        if (params.task & AllocateTask_Copy_String)
            string_copy = string_copy_from_other(params.string);

        // .Initialize
        if (params.task & AllocateTask_Initialize) {
            // TODO: assert mandatory params for this section
            //
            ((Object*)object_st)->kind = ObjectKind_String;
            LinkedList_push(*params.first, (Object*)object_st);
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

ObjectFunction* ObjectFunction_allocate(Object** object_head) {
    ObjectFunction* function = calloc(1, sizeof(ObjectFunction));
    assert(function);

    return function;
}

ObjectValue* ObjectValue_allocate(Object** object_head, Value* value_address) {
    ObjectValue* object_value = calloc(1, sizeof(ObjectValue));
    assert(object_value);

    ((Object*)object_value)->kind = ObjectKind_Heap_Value;
    LinkedList_push(*object_head, (Object*)object_value);
    object_value->value_address = value_address;
    object_value->value = value_make_nil(); 

    return object_value;
}

ObjectClosure* ObjectClosure_allocate(ObjectFunction* function, Object** object_head) {
    ObjectClosure* closure = calloc(1, sizeof(ObjectClosure));
    assert(closure);

    // Initialization
    //
    ((Object*)closure)->kind =  ObjectKind_Closure;
    LinkedList_push(*object_head, (Object*)closure);
    closure->function = function;

    int item_count = function->variable_dependencies_count;
    ObjectValue** items = malloc(item_count * sizeof(ObjectValue*));
    assert(items);
    for (int i = 0; i < item_count; i++) items[i] = NULL;

    closure->heap_values.items = items;
    closure->heap_values.count = item_count;
    return closure;
}

ObjectFunctionNative* ObjectFunctionNative_allocate(FunctionNative* function, Object** object_head, int arity) {
    ObjectFunctionNative* native = calloc(1, sizeof(ObjectFunctionNative));
    assert(native);
    
    ((Object*)native)->kind = ObjectKind_Function_Native;
    LinkedList_push(*object_head, (Object*)native);
    native->function = function;
    native->arity = arity;

    return native;
}

void Object_init(Object* object, ObjectKind kind, Object** object_head) {
    object->kind = kind;
    if (object_head != NULL) LinkedList_push(*object_head, object);

    // TODO: change if clause to 'LinkedList_push(object_head, object);'
    //
    // if (object_head != NULL) {
    //     object->next = *object_head;
    //     *object_head = object;
    // }
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

void Object_free(Object* object) {
    // TODO: update virtual machine's bytes allocated variable
    switch (object->kind)
    {
    case ObjectKind_String: {
        // TODO: Missing Implementation
        // ObjectString_free((ObjectString*)object);
        //
        assert(false && "[ERROR] object.c:157 Missing Implementation");
    } break;
    case ObjectKind_Function: {
        // TODO: Missing Implementation
        // ObjectFunction_free((ObjectFunction*)object);
        // 
        assert(false && "[ERROR] object.c:164 Missing Implementation");
    } break;
    case ObjectKind_Function_Native: {
        // TODO: Missing Implementation
        // ObjectFunctionNative_free((ObjectFunctionNative*)object);
        // 
        assert(false && "[ERROR] object.c:171 Missing Implementation");
    } break;
    }
}
