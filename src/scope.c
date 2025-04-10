#include "kriolu.h"

void scope_init(Scope* scope) {
    StackLocal_init(&scope->locals, UINT8_COUNT);
    scope->depth;
}
