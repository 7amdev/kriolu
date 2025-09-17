#ifndef Stack_H
#define Stack_H

#ifndef Stack_Free
#include <stdlib.h>
#define Stack_Free free
#endif // Stack_Free

#ifndef Stack_Assert 
#include <assert.h>
#define Stack_Assert assert
#endif // Stack_Assert

#define Stack_Items_Length(items) (sizeof(items) / sizeof(*(items)))

#define Stack_push(stack, item_in, item_out)                                    \
do {                                                                            \
    Stack_Assert(Stack_is_full(stack) == false && "ERROR: Stack is full!");     \
    (stack)->items[(stack)->top] = (item_in);                                   \
    (stack)->top += 1;                                                          \
    if ((item_out) != NULL) *(item_out) = (stack)->items[(stack)->top - 1];     \
} while (0)

#define Stack_pop(stack, item_out)                                              \
do {                                                                            \
    Stack_Assert(Stack_is_empty(stack) == false && "ERROR: Stack is empty!");   \
    (stack)->top -= 1;                                                          \
    if ((item_out) != NULL) *(item_out) = (stack)->items[(stack)->top];         \
} while (0)

#define Stack_peek(stack, item_out, offset)                         \
do {                                                                \
    int index = (stack)->top - 1 - (offset);                        \
    Stack_Assert(index >= 0);                                       \
    Stack_Assert(index < (stack)->top);                             \
    if ((item_out) != NULL) *(item_out) = (stack)->items[index];    \
} while (0)

#define Stack_is_full(stack)  ((stack)->top == Stack_Items_Length((stack)->items))

#define Stack_is_empty(stack) ((stack)->top == 0)

#define Stack_is_out_of_bounds(stack, offset) (         \
    ((stack)->top - 1 - offset) < 0             ||      \
    ((stack)->top - 1 - offset) >= (stack)->top         \
)

#define Stack_reset(stack)    (stack)->top = 0

#define Stack_free(stack) Stack_Free(stack)

// 
// Stack Pointer Implementation
// 

#define StackPointer_count(stack)    (size_t)(  (stack)->top - (stack)->items  )
#define StackPointer_is_full(stack)  ( ((stack)->top - (stack)->items) == Stack_Items_Length((stack)->items) )
#define StackPointer_is_empty(stack) ( ((stack)->top - (stack)->items) == 0 )

#define StackPointer_init(stack)    \
do {                                \
    (stack)->top = (stack)->items;  \
} while (0)

#define StackPointer_reset(stack)  StackPointer_init(stack)

#define StackPointer_push(stack, item_in, item_out)         \
do {                                                        \
    Stack_Assert(!StackPointer_is_full(stack));             \
    *(stack)->top = (item_in);                              \
    (stack)->top += 1;                                      \
    if (item_out) *(item_out) = *(stack)->top;              \
} while (0)

#define StackPointer_pop(stack, item_out)                   \
do {                                                        \
    Stack_Assert(!StackPointer_is_empty(stack));            \
    (stack)->top -= 1;                                      \
    if (item_out) *(item_out) = *(stack)->top;              \
} while (0)

typedef struct {
    int offset;
    int *error;
    const char **error_msg;
} PeekOptions;

#define StackPointer_peek(stack, item_out, ...)     \
    StackPointer_peek_option(                       \
        (stack),                                    \
        (item_out),                                 \
        (PeekOptions) {__VA_ARGS__}                 \
    )

#define StackPointer_peek_option(stack, item_out, options)              \
do {                                                                    \
    void *addr = (stack)->top - 1 - (options).offset;                   \
    if (addr < (stack)->items || addr >= (stack)->top) {                \
        if ((options).error) *(options).error = 1;                      \
        if ((options).error_msg)                                        \
        *(options).error_msg = "Out-of-Bounds access lookup.";      \
        break;                                                          \
    }                                                                   \
    if ((options).error_msg) *(options).error_msg = "";                 \
    if ((options).error) *(options).error = 0;                          \
    if (item_out) *item_out = *((stack)->top - 1 - (options).offset);   \
} while (0)


#define StackPointer_free(stack)     Stack_Free(stack)

#endif // Stack_H