#include "kriolu.h"

void StackLocal_init(StackLocal* locals) {
    // locals->items = malloc(capacity * sizeof(Local));
    assert(locals->items);
    locals->top = 0;
    locals->capacity = UINT8_COUNT;
}

Local StackLocal_push(StackLocal* locals, Token token, int scope_depth, LocalAction action) {
    assert(locals->top < locals->capacity && "Error: StackLocal Overflow.");

    locals->items[locals->top] = (Local){ 
        .token       = token, 
        .scope_depth = scope_depth, 
        .action      = action 
    };
    locals->top += 1;

    return locals->items[locals->top - 1];
}

Local StackLocal_pop(StackLocal* locals) {
    assert(locals->top > 0 && "Error: StackLocal underflow.");

    locals->top -= 1;
    Local local = locals->items[locals->top];
    locals->items[locals->top] = (Local){ .token = {0}, .scope_depth = 0 };

    return local;
}

Local* StackLocal_peek(StackLocal* locals, int offset) {
    int index = locals->top - 1 - offset;
    assert(index > -1 && "Error: Index out of bond!");
    assert(index < locals->capacity);

    return &locals->items[index];
}

int StackLocal_get_local_index_by_token(StackLocal* locals, Token* token, Local** local_out) {
    if (locals == NULL) return -1;

    for (int i = locals->top - 1; i >= 0; i--) {
        Local* local = &locals->items[i];
        if (token_is_identifier_equal(&local->token, token)) {
            if (local_out != NULL) *local_out = local;
            return i;
        }
    }

    return -1;
}

bool StackLocal_is_full(StackLocal* locals) {
    return (locals->top == locals->capacity);
}

bool StackLocal_is_empty(StackLocal* locals) {
    return (locals->top == 0);
}

bool StackLocal_initialize_local(StackLocal* locals, int depth, int offset) {
    Local* local = StackLocal_peek(locals, offset);
    if (local == NULL) return false;

    local->scope_depth = depth;
    return true;;
}