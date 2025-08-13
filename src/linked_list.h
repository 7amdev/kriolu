#ifndef LinkedList_H
#define LinkedList_H

#define Link(Type) Type##*
#define LinkedList(Type) Type##*
#define LinkedListHeader(Type) Type##* next

#define LinkedList_push(first, node) do {   \
    (node)->next = (first);                 \
    (first) = (node);                       \
} while (0)

#define LinkedList_pop(first, node_out) do {      \
    (node_out) = (first);                         \
    (first) = (first)->next;                      \
} while (0)

// [first_in]  A pointer to the starting Node.
// [offset_in] Can be a variable or a literal value.
//
#define LinkedList_peek(Type, first_in, offset_in, node_out) do {           \
    Type* loop  = (first_in);                                               \
    int _offset = (offset_in);                                              \
    for (; _offset > 0 && loop != NULL; loop = loop->next, _offset -= 1);   \
    if (_offset != 0) { *(node_out) = (Type){0}; break; }                   \
    if (loop == NULL) { *(node_out) = (Type){0}; break; }                   \
    *(node_out) = *loop;                                                    \
} while (0)

#define LinkedList_peek_top(Type, first, node_out) \
        LinkedList_peek(Type, first, 0, node_out) 

#define LinkedList_insert_after(base_node, new_node) do {               \
    (new_node)->next = (base_node)->next;                               \
    (base_node)->next = (new_node);                                     \
} while (0)

// TODO: #define LinkedList_insert_at(...)
// TODO: #define LinkedList_remove_at(...)

#define LinkedList_foreach(Type, node, it)                                  \
    for (                                                                   \
        struct { Type* curr; Type* prev; int i; } it = { (node), NULL, 0};  \
        (it).curr != NULL;                                                  \
        (it).prev = (it).curr, (it).curr = (it).curr->next, (it).i += 1     \
    )

#define LinkedList_is_empty(first) (first == NULL)

#endif // LinkedList_H

#ifdef LinkedList_Implementation
#endif // LinkedList_Implementation