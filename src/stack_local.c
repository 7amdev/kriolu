#include "kriolu.h"


void StackLocal_init(StackLocal* locals, int capacity) {
    locals->items = malloc(capacity * sizeof(Local));
    assert(locals->items);
    locals->count = 0;
    locals->capacity = capacity;
}

Local StackLocal_push(StackLocal* locals, Token token, int scope_depth) {
    assert(locals->count < locals->capacity && "Error: StackLocal Overflow.");
    // Local* local = &locals->items[locals->count];
    // local->token = token;
    // local->scope_depth = scope_depth;
    // locals->count += 1;

    Local local = (Local){ token, scope_depth };
    locals->items[locals->count] = local;
    locals->count += 1;

    return locals->items[locals->count - 1];

}

Local StackLocal_pop(StackLocal* locals) {
    assert(locals->count > 0 && "Error: StackLocal underflow.");

    Local* local = &locals->items[locals->count];
    locals->count -= 1;

    return *local;
}

int StackLocal_get_local_index_by_token(StackLocal* locals, Token* token) {
    return -1;
}

bool StackLocal_is_full(StackLocal* locals) {
    return locals->count == locals->capacity;
}