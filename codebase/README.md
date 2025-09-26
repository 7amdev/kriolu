# Codebase
A collection of useful libraries for C programming language.

All the libraries is implemented in a 'stb-style header-only library' giving more flexibity and control on how include into a project.

## Basic Usage
For more examples on how to use the libraries, check the folder './tests/'.

Copy the Codebase folder inside your project.

```c
// Folder Structure
// 
Project/
|-- build/
|-- codebase/
|   |-- c_stack.h
|   |-- c_linked_list.h
|   |-- ... 
|-- src/
|   |-- main.c
|-- build.bat

// src/main.c
// 
#include <stdio.h>

#define  Stack_Implementation
#include "codebase/c_stack.h"

typedef struct {
    int   id;
    char *name;
    float value;
} Data;

typedef struct {
    int   top;
    Data *items;
} StackData;

int main(void) {
    Stack stack =  {0};
    Data d1  = { .id = 1, .name = "John Doe", .value = 1.67f };
    Data d2  = { .id = 2, .name = "Jane Doe", .value = 1.52f };
    Data ret = {0};

    Stack_push(&stack, d1, &ret);
    Stack_push(&stack, d2, &ret);

    Stack_pop(&stack, &ret);

    for (int i = 0; i < stack.top; i++) {
        // Print data...
    }

    return 0;
}

// build.bat
//
cl main.c /link /out:main.exe
```

## Build a particular library in its own Translation Unit
### Windows 
```c
// Terminal: create a new file
// 
echo. > stack.c

// stack.c
//
#define  Stack_Implementation
#include "codebase/c_stack.h"

// main.c
// 
#include <stdio.h>
#include "codebase/c_stack.h"

// ... 

int main(void) {
    // ...

    Stack_push(&stack, d1, &ret);

    // ...

    return 0;
}

// build.bat
//
cl main.c stack.c /link /out:main.exe
```

## Testing

Open a terminal and follow the commands bellow:

```shell
cd tests/
build.bat
cd build/
tests.exe
```
### Output
Expected output:

```shell
[PASS] DynamicArray_test_capacity()
[PASS] DynamicArray_test_append()
[PASS] DynamicArray_test_append_many()
[PASS] DynamicArray_test_append_strings()
[PASS] DynamicArray_test_append_string_ptr()
[PASS] Queue_test_push_back()
[PASS] Queue_test_pop_front()
```