#include "kriolu.h"


// TODO: Change prefix from Stack to Array

void StackLocal_init(StackLocal* locals, int capacity) {
    locals->items = malloc(capacity * sizeof(Local));
    assert(locals->items);
    locals->top = 0;
    locals->capacity = capacity;
}

Local StackLocal_push(StackLocal* locals, Token token, int scope_depth) {
    assert(locals->top < locals->capacity && "Error: StackLocal Overflow.");
    // Local* local = &locals->items[locals->count];
    // local->token = token;
    // local->scope_depth = scope_depth;
    // locals->count += 1;

    Local local = (Local){ token, scope_depth };
    locals->items[locals->top] = local;
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

Local* StackLocal_peek(StackLocal* local, int offset) {
    int index = local->top - 1 - offset;
    assert(index > -1);
    assert(index < local->capacity);

    return &local->items[index];
}

int StackLocal_get_local_index_by_token(StackLocal* locals, Token* token, Local** local_out) {
    for (int i = locals->top - 1; i >= 0; i--) {
        Local* local = &locals->items[i];
        if (token_is_identifier_equal(&local->token, token)) {
            *local_out = local;
            return i;
        }
    }

    return -1;
}

bool StackLocal_is_full(StackLocal* locals) {
    return locals->top == locals->capacity;
}