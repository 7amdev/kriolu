#ifndef Queue_H
#define Queue_H

#ifndef Queue_Initial_Capacity
#define Queue_Initial_Capacity 256
#endif // Queue_Initial_Capacity

#ifndef Queue_Realloc 
#include <stdlib.h>
#define Queue_Realloc realloc
#endif // Queue_Realloc

#ifndef Queue_Free 
#include <stdlib.h>
#define Queue_Free free
#endif // Queue_Free

#ifndef Queue_Assert 
#include <assert.h>
#define Queue_Assert assert
#endif // Queue_Assert

#define Queue(Type) struct { Type* first; Type* last; }

#define Queue_Push_Back(first, last, node, node_member_next) do {   \
    if ((first) == NULL) {                                            \
        (first) = (node);                                           \
        (last)  = (node);                                           \
        (node)->node_member_next = NULL;                            \
        break;                                                      \
    }                                                               \
    (node)->node_member_next = NULL;                                \
    (last)->node_member_next = (node);                              \
    (last)                   = (node);                              \
} while (0)

#define Queue_Pop_Front(first, last, node_member_next) do {         \
    if ((first) == (last)) {                                        \
        (first) = NULL;                                             \
        (last)  = NULL;                                             \
        break;                                                      \
    }                                                               \
    (first) = (first)->node_member_next;                            \
} while (0)

// Default API
//
#define Queue_push_back(first, last, node)  Queue_Push_Back((first), (last), (node), next)
#define Queue_pop_front(first, last)        Queue_Pop_Front((first), (last), next)
#define Queue_is_empty(first, last) ((first) == (last))

#endif // Queue_H