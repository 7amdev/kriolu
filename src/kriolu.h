#ifndef kriolu_h
#define kriolu_h

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <math.h>
#include <stdarg.h>

#define DEBUG_LOG_PARSER

// todo: change Array, Stack, and Ast naming convention to:
//       AstExpression, StackValue, ArrayInstruction, ArrayStatement, ...

//
// Token
//

typedef enum
{
    TOKEN_ERROR,
    TOKEN_EOF,

    TOKEN_LEFT_PARENTHESIS,  // (
    TOKEN_RIGHT_PARENTHESIS, // )
    TOKEN_LEFT_BRACE,        // {
    TOKEN_RIGHT_BRACE,       // }
    TOKEN_COMMA,
    TOKEN_DOT,
    TOKEN_MINUS,
    TOKEN_PLUS,
    TOKEN_SEMICOLON,
    TOKEN_SLASH,
    TOKEN_ASTERISK,
    TOKEN_CARET,

    TOKEN_EQUAL,         // =
    TOKEN_EQUAL_EQUAL,   // ==
    TOKEN_NOT_EQUAL,     // =/=
    TOKEN_GREATER,       // >
    TOKEN_GREATER_EQUAL, // >=
    TOKEN_LESS,          // <
    TOKEN_LESS_EQUAL,    // <=

    TOKEN_IDENTIFIER,
    TOKEN_STRING,
    TOKEN_STRING_INTERPOLATION,
    TOKEN_NUMBER,

    TOKEN_E,  // logic operator AND
    TOKEN_OU, // logic operator OR
    TOKEN_KA, // logic operator NOT
    TOKEN_KLASI,
    TOKEN_SI,
    TOKEN_SINOU,
    TOKEN_FALSU,
    TOKEN_VERDADI,
    TOKEN_DI,
    TOKEN_FUNSON,
    TOKEN_NULO,
    TOKEN_IMPRIMI,
    TOKEN_DIVOLVI,
    TOKEN_SUPER,
    TOKEN_KELI,    // this
    TOKEN_MIMORIA, // variable
    TOKEN_TIMENTI,
    TOKEN_TI,

    TOKEN_COMMENT
} TokenKind;

typedef struct
{
    TokenKind kind;
    const char* start;
    int length;
    int line_number;
} Token;

//
// Lexer
//

#define STRING_INTERPOLATION_MAX 8

typedef struct
{
    const char* start;
    const char* current;
    int line_number;

    int string_nested_interpolation[STRING_INTERPOLATION_MAX];
    int string_interpolation_count;

} Lexer;

#define l_debug_print_token(token) lexer_debug_print_token(token, "%s ")

Lexer* lexer_create_static();
void lexer_init(Lexer* lexer, const char* source_code);
Token lexer_scan(Lexer* lexer);
void lexer_debug_print_token(Token token, const char* format);
void lexer_debug_dump_tokens(Lexer* lexer);
void lexer_destroy_static(Lexer* lexer);

//
// String
//

// String is a sequence of characters.
//
typedef struct
{
    char* characters;
    int length;
} String;

// sizeof("Hello") -> 6: it counts the character '\0', so
// the -1 at the end.
//
#define string_make_from_literal(string) \
    (String){.characters = (char *)string, .length = sizeof(string) - 1}
#define string_make_from_array(string) \
    (String) { .characters = (char *)string, .length = strlen(string) }

String* string_allocate(const char* characters, int length);
String string_make(const char* characters, int length);
String string_make_from_format(const char* format, ...);
String string_make_and_copy_characters(const char* characters, int length);
void string_initialize(String* string, const char* characters, int length);
String string_copy(String other);
String string_concatenate(String a, String b);
uint32_t string_hash(String string);
bool string_equal(String a, String b);
void string_free(String* string);

//
// Object
//
// TODO: Move HashTable forward declaration to the top of the file 
typedef struct HashTable HashTable;
typedef struct Object Object;

typedef enum
{
    ObjectKind_Invalid,

    ObjectKind_String
} ObjectKind;

struct Object
{
    ObjectKind kind;

    // Access next object on the linked list
    //
    Object* next;
};

typedef struct
{
    Object object;

    // String
    //
    char* characters;
    int length;

    uint32_t hash;
} ObjectString;

#define object_cast_to_string(object) ((ObjectString *)object)
#define string_cast_to_object(string) ((Object *)string)

#define object_get_string_chars(object) (object_cast_to_string(object)->characters)

Object* object_allocate(ObjectKind kind, size_t size);
void object_init(Object* object, ObjectKind kind);
void object_clear(Object* object);
void object_print(Object* object);
void object_free(Object* object);

#define object_create_and_intern_string(characters, length, hash) object_allocate_and_intern_string(&g_vm.strings, characters, length, hash)

ObjectString* object_allocate_string(char* characters, int length, uint32_t hash);
ObjectString* object_allocate_and_intern_string(HashTable* table, char* characters, int length, uint32_t hash);
String object_to_string(ObjectString* object_string);
void object_free_string(ObjectString* string);

//
// Value
//

typedef enum
{
    Value_Boolean,
    Value_Nil,
    Value_Number,
    Value_Object,

    Value_Count
} ValueKind;

typedef struct
{
    ValueKind kind;
    union
    {
        bool boolean;
        double number;
        Object* object;
    } as;
} Value;

typedef struct
{
    Value* items;
    int count;
    int capacity;
} ValueArray;

#define value_make_boolean(value) ((Value){.kind = Value_Boolean, .as = {.boolean = value}})
#define value_make_number(value) ((Value){.kind = Value_Number, .as = {.number = value}})
#define value_make_object(value) ((Value){.kind = Value_Object, .as = {.object = (Object *)value}})
#define value_make_object_string(value) ((Value){.kind = Value_Object, .as = {.object = (Object *)value}})
#define value_make_nil() ((Value){.kind = Value_Nil, .as = {.number = 0}})

#define value_as_boolean(value) ((value).as.boolean)
#define value_as_number(value) ((value).as.number)
#define value_as_nil(value) ((value).as.number)
#define value_as_object(value) ((value).as.object)
#define value_as_string(value) ((ObjectString *)value_as_object(value))

#define value_is_boolean(value) ((value).kind == Value_Boolean)
#define value_is_number(value) ((value).kind == Value_Number)
#define value_is_object(value) ((value).kind == Value_Object)
#define value_is_nil(value) ((value).kind == Value_Nil)
#define value_is_string(value) value_is_object_type(value, ObjectKind_String)

#define value_get_object_type(value) (value_as_object(value)->type)
#define value_get_string_chars(value) (value_as_string(value)->characters)

bool value_negate_logically(Value value);
bool value_is_equal(Value a, Value b);
inline bool value_is_object_type(Value value, ObjectKind object_kind);

void value_array_init(ValueArray* values);
uint32_t value_array_insert(ValueArray* values, Value value);
void value_print(Value value);
void value_array_free(ValueArray* values);

//
// Abstract Syntax Tree
//

typedef uint8_t ExpressionKind;
enum
{
    ExpressionKind_Invalid,

    ExpressionKind_Number,
    ExpressionKind_Boolean,
    ExpressionKind_String,
    ExpressionKind_Nil,
    ExpressionKind_Negation,
    ExpressionKind_Not,
    ExpressionKind_Grouping,
    ExpressionKind_Addition,
    ExpressionKind_Subtraction,
    ExpressionKind_Multiplication,
    ExpressionKind_Division,
    ExpressionKind_Exponentiation,
    ExpressionKind_Equal_To,
    ExpressionKind_Greater_Than,
    ExpressionKind_Less_Than
};

typedef struct Expression Expression;
struct Expression
{
    ExpressionKind kind;
    union
    {
        double number;
        bool boolean;
        Object* object;
        struct
        {
            Expression* operand;
        } unary;
        struct
        {
            Expression* left;
            Expression* right;
        } binary;
    } as;
};

#define expression_make_number(value) ((Expression){.kind = ExpressionKind_Number, .as = {.number = (value)}})
#define expression_make_boolean(value) ((Expression){.kind = ExpressionKind_Boolean, .as = {.boolean = (value)}})
#define expression_make_string(value) ((Expression){.kind = ExpressionKind_String, .as = {.object = (Object *)value}})
#define expression_make_nil() ((Expression){.kind = ExpressionKind_Nil, .as = {.number = 0}})
#define expression_make_negation(expression) ((Expression){.kind = ExpressionKind_Negation, .as = {.unary = {.operand = (expression)}}})
#define expression_make_not(expression) ((Expression){.kind = ExpressionKind_Not, .as = {.unary = {.operand = (expression)}}})
#define expression_make_grouping(expression) ((Expression){.kind = ExpressionKind_Grouping, .as = {.unary = {.operand = (expression)}}})
#define expression_make_addition(left_operand, right_operand) ((Expression){.kind = ExpressionKind_Addition, .as = {.binary = {.left = (left_operand), .right = (right_operand)}}})
#define expression_make_subtraction(left_operand, right_operand) ((Expression){.kind = ExpressionKind_Subtraction, .as = {.binary = {.left = (left_operand), .right = (right_operand)}}})
#define expression_make_multiplication(left_operand, right_operand) ((Expression){.kind = ExpressionKind_Multiplication, .as = {.binary = {.left = (left_operand), .right = (right_operand)}}})
#define expression_make_division(left_operand, right_operand) ((Expression){.kind = ExpressionKind_Division, .as = {.binary = {.left = (left_operand), .right = (right_operand)}}})
#define expression_make_exponentiation(left_operand, right_operand) ((Expression){.kind = ExpressionKind_Exponentiation, .as = {.binary = {.left = (left_operand), .right = (right_operand)}}})
#define expression_make_equal_to(left_operand, right_operand) ((Expression){.kind = ExpressionKind_Equal_To, .as = {.binary = {.left = (left_operand), .right = (right_operand)}}})
#define expression_make_greater_than(left_operand, right_operand) ((Expression){.kind = ExpressionKind_Greater_Than, .as = {.binary = {.left = (left_operand), .right = (right_operand)}}})
#define expression_make_less_than(left_operand, right_operand) ((Expression){.kind = ExpressionKind_Less_Than, .as = {.binary = {.left = (left_operand), .right = (right_operand)}}})

#define expression_as_number(expression) ((expression).as.number)
#define expression_as_boolean(expression) ((expression).as.boolean)
#define expression_as_object(expression) ((expression).as.object)
#define expression_as_string(expression) ((ObjectString *)(expression).as.object)
#define expression_as_value(expression) ((expression).as.value)
#define expression_as_negation(expression) ((expression).as.unary)
#define expression_as_not(expression) ((expression).as.unary)
#define expression_as_grouping(expression) ((expression).as.unary)
#define expression_as_binary(expression) ((expression).as.binary)
#define expression_as_addition(expression) ((expression).as.binary)
#define expression_as_subtraction(expression) ((expression).as.binary)
#define expression_as_multiplication(expression) ((expression).as.binary)
#define expression_as_division(expression) ((expression).as.binary)
#define expression_as_exponentiation(expression) ((expression).as.binary)
#define expression_as_equal(expression) ((expression).as.binary)
#define expression_as_greater(expression) ((expression).as.binary)
#define expression_as_less(expression) ((expression).as.binary)

Expression* expression_allocate(Expression expr);
void expression_print(Expression* expression);
void expression_print_tree(Expression* expression, int indent);
void expression_free(Expression* expression);

typedef uint8_t StatementKind;
enum
{
    StatementKind_Invalid,

    StatementKind_Expression,
    StatementKind_Block,
    StatementKind_Return,
    StatementKind_Si,
    StatementKind_Di,
};

typedef struct Statement Statement;
struct Statement
{
    StatementKind kind;
    union
    {
        Expression* expression;
    };
};

typedef struct
{
    Statement* items;
    uint32_t count;
    uint32_t capacity;
} StatementArray;

StatementArray* statement_array_allocate();
uint32_t statement_array_insert(StatementArray* statements, Statement statement);
void statement_array_free(StatementArray* statements);

//
// Parser
//

typedef struct
{
    Token current;  // TODO: rename to current_token
    Token previous; // TODO: rename to previous_token
    Lexer* lexer;
    bool had_error;
    bool panic_mode;
    int interpolation_count_nesting;
    int interpolation_count_value_pushed;
} Parser;

#define parser_init(parser, source_code) parser_initialize(parser, source_code, NULL)

void parser_initialize(Parser* parser, const char* source_code, Lexer* lexer);
StatementArray* parser_parse(Parser* parser);

//
// Line Number
//

typedef struct
{
    int* items;
    int count;
    int capacity;
} LineNumberArray;

void line_array_init(LineNumberArray* lines);
int line_array_insert(LineNumberArray* lines, int line);
int line_array_insert_3x(LineNumberArray* lines, int line);
void line_array_free(LineNumberArray* lines);

//
// Instruction
//

typedef uint8_t OpCode;
enum
{
    OpCode_Invalid,

    OpCode_Constant,
    OpCode_Constant_Long,
    OpCode_Interpolation,
    OpCode_Nil,
    OpCode_True,
    OpCode_False,
    OpCode_Negation,
    OpCode_Not,
    OpCode_Addition,
    OpCode_Subtraction,
    OpCode_Multiplication,
    OpCode_Division,
    // TODO: add OpCode_Modulus
    OpCode_Exponentiation,
    OpCode_Equal_To,
    OpCode_Greater_Than,
    OpCode_Less_Than,

    OpCode_Return
};

typedef struct
{
    uint8_t* items;
    int count;
    int capacity;
} InstructionArray;

void instruction_array_init(InstructionArray* instructions);
int instruction_array_insert(InstructionArray* instructions, uint8_t item);
int instruction_array_insert_u24(InstructionArray* instructions, uint8_t byte1, uint8_t byte2, uint8_t byte3);
void instruction_array_free(InstructionArray* instructions);

//
// Bytecode
//

// Bytecode is a series of instructions. Eventually, we'll store
// some other data along with instructions, so ...
//
typedef struct
{
    InstructionArray instructions;
    LineNumberArray lines;
    ValueArray values;
} Bytecode;

extern Bytecode g_bytecode;
#define bytecode_emit_byte(data, line) bytecode_write_byte(&g_bytecode, data, line)
#define bytecode_emit_constant(value, line) bytecode_write_constant(&g_bytecode, value, line)

#define bytecode_write_opcode(bytecode, opcode, line_number) bytecode_write_byte(bytecode, opcode, line_number)
#define bytecode_write_operand_u8(bytecode, operand, line_number) bytecode_write_byte(bytecode, operand, line_number)
#define bytecode_write_operand_u24(bytecode, byte1, byte2, byte3, line_number) bytecode_write_instruction_u24(bytecode, byte1, byte2, byte3, line_number)

void bytecode_init(Bytecode* bytecode);
int bytecode_write_byte(Bytecode* bytecode, uint8_t data, int line_number);
int bytecode_write_constant(Bytecode* bytecode, Value value, int line_number);
void bytecode_disassemble(Bytecode* bytecode, const char* name);
int bytecode_disassemble_instruction(Bytecode* bytecode, int offset);
void bytecode_emitter_begin();
Bytecode bytecode_emitter_end();
void bytecode_free(Bytecode* bytecode);

//
// Value Stack
//

#define STACK_MAX 256

typedef struct
{
    Value items[STACK_MAX];
    Value* top;
} StackValue;

StackValue* stack_value_create(void);
void stack_value_reset(StackValue* stack);
Value stack_value_push(StackValue* stack, Value value);
Value stack_value_pop(StackValue* stack);
Value stack_value_peek(StackValue* stack, int offset);
bool stack_value_is_full(StackValue* stack);
bool stack_value_is_empty(StackValue* stack);
void stack_value_trace(StackValue* stack);
void stack_free(StackValue* stack);

//
// HashTable
//
// HashTable:
//     Associates a set of Keys to a set of Values.
//     Each Key/Value pair is an Entry in the Table.
//     Constant time lookup.
//
//  HashTable
// key  | Value
// ---------------
// 1248   "Hello"  -> Entry  <- *entries
// 4323    9892    -> Entry
//
// Entries: [{1248, "Hello"}, {4323, 9892}, ...]
//           ---------------  ------------
//                Entry           Entry

typedef struct
{
    ObjectString* key;
    Value value;
} Entry;

struct HashTable
{
    Entry* entries;
    int count;
    int capacity;
};

void hash_table_init(HashTable* table);
void hash_table_copy(HashTable* from, HashTable* to); // tableAddAll
bool hash_table_set_value(HashTable* table, ObjectString* key, Value value);
bool hash_table_get_value(HashTable* table, ObjectString* key, Value* value_out);
ObjectString* hash_table_get_key(HashTable* table, String string, uint32_t hash);
bool hash_table_delete(HashTable* table, ObjectString* key);
void hash_table_free(HashTable* table);

//
// Virtual Machine
//

#define DEBUG_TRACE_EXECUTION

typedef enum
{
    Interpreter_Ok,
    Interpreter_Compiler_error,
    Interpreter_Runtime_Error
} InterpreterResult;

typedef struct
{
    Bytecode* bytecode;
    StackValue stack_value;

    // Instruction Pointer
    //
    uint8_t* ip;

    // A linked list of all objects created at runtime
    // TODO: move this variable to object.c module
    Object* objects;

    // Stores all unique strings allocated during runtime
    //
    HashTable strings;

    // TODO: add a varible to track total bytes allocated
} VirtualMachine;

extern VirtualMachine g_vm;

#define vm_init(bytecode) virtual_machine_init(&g_vm, bytecode)
#define vm_interpret() virtual_machine_interpret(&g_vm)
#define vm_free() virtual_machine_free(&g_vm)

void virtual_machine_init(VirtualMachine* vm, Bytecode* bytecode);
InterpreterResult virtual_machine_interpret(VirtualMachine* vm);
void virtual_machine_free(VirtualMachine* vm);

#endif