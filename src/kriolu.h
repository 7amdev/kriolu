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

// 
// Utils
// 

// #define block(condition) for (int i = 0;i < 1 && (condition); ++i)
#define block for (int i = 0;i < 1; ++i)
#define DEBUG_LOG_PARSER

//
// Token
//

typedef enum
{
    Token_Error, // TODO: change to token_invalid
    Token_Eof,

    Token_Left_Parenthesis,  // (
    Token_Right_Parenthesis, // )
    Token_Left_Brace,        // {
    Token_Right_Brace,       // }
    Token_Comma,
    Token_Dot,
    Token_Minus,
    Token_Plus,
    Token_Semicolon,
    Token_Slash,
    Token_Asterisk,
    Token_Caret,

    Token_Equal,         // =
    Token_Equal_Equal,   // ==
    Token_Not_Equal,     // =/=
    Token_Greater,       // >
    Token_Greater_Equal, // >=
    Token_Less,          // <
    Token_Less_Equal,    // <=

    Token_Identifier,
    Token_String,
    Token_String_Interpolation,
    Token_Number,

    Token_E,  // logic operator AND
    Token_Ou, // LOGIC OPERATOR or
    Token_Ka, // LOGIC OPERATOR not
    Token_Klasi,
    Token_Si,
    Token_Sinou,
    Token_Falsu,
    Token_Verdadi,
    Token_Di,
    Token_Funson,
    Token_Nulo,
    Token_Imprimi,
    Token_Divolvi,
    Token_Super,
    Token_Keli,    // THIS
    Token_Mimoria, // VARIABLE
    Token_Timenti,
    Token_Ti,

    Token_Comment
} TokenKind;

typedef struct
{
    TokenKind kind;
    const char* start;
    int length;
    int line_number;
} Token;

bool token_is_identifier_equal(Token* a, Token* b);

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

// sizeof("Hello") -> 6: it counts the character '\0', hence
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

    // String related fields
    //
    char* characters;
    int length;
    uint32_t hash;

} ObjectString;

Object* Object_allocate(ObjectKind kind, size_t size);
Object* Object_allocate_string(char* characters, int length, uint32_t hash);
void Object_init(Object* object, ObjectKind kind);
void Object_clear(Object* object);
void Object_print(Object* object);
void Object_free(Object* object);

#define ObjectString_AllocateAndIntern(characters, length, hash) ObjectString_allocate_and_intern(&g_vm.string_database, characters, length, hash)
#define ObjectString_AllocateIfNotInterned(characters, length) ObjectString_allocate_if_not_interned(&g_vm.string_database, characters, length)

ObjectString* ObjectString_allocate(char* characters, int length, uint32_t hash);
ObjectString* ObjectString_allocate_if_not_interned(HashTable* table, const char* characters, int length);
ObjectString* ObjectString_allocate_and_intern(HashTable* table, char* characters, int length, uint32_t hash);
void ObjectString_init(ObjectString* object_string, char* characters, int length, uint32_t hash);
String ObjectString_to_string(ObjectString* object_string);
void ObjectString_free(ObjectString* string);

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
} ArrayValue;

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
bool value_is_falsey(Value value);
bool value_is_equal(Value a, Value b);
void value_print(Value value);
inline bool value_is_object_type(Value value, ObjectKind object_kind) {
    return value_is_object(value) && value_as_object(value)->kind == object_kind;
}


void array_value_init(ArrayValue* values);
uint32_t array_value_insert(ArrayValue* values, Value value);
void array_value_free(ArrayValue* values);

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
    ExpressionKind_Greater_Than_Or_Equal_To,
    ExpressionKind_Less_Than,
    ExpressionKind_Less_Than_Or_Equal_To,
    ExpressionKind_Assignment,
    ExpressionKind_Variable
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
        ObjectString* variable;
        struct { Expression* operand; } unary;
        struct { Expression* left; Expression* right; } binary;
        struct { ObjectString* lhs; Expression* rhs; } assignment; // lhs -> Left Hand Side; rhs -> Right Hand Side
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
#define expression_make_greater_than_or_equal_to(left_operand, right_operand) ((Expression){.kind = ExpressionKind_Greater_Than_Or_Equal_To, .as = {.binary = {.left = (left_operand), .right = (right_operand)}}})
#define expression_make_less_than(left_operand, right_operand) ((Expression){.kind = ExpressionKind_Less_Than, .as = {.binary = {.left = (left_operand), .right = (right_operand)}}})
#define expression_make_less_than_or_equal_to(left_operand, right_operand) ((Expression){.kind = ExpressionKind_Less_Than_Or_Equal_To, .as = {.binary = {.left = (left_operand), .right = (right_operand)}}})
#define expression_make_variable(variable) ((Expression) {.kind = ExpressionKind_Variable, .as = {.variable = (variable)}})
#define expression_make_assignment(lhs, rhs) ((Expression){.kind = ExpressionKind_Assignment, .as = {.assignment = {.lhs = (lhs), .rhs = (rhs)}}})

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
#define expression_as_variable(expression) ((expression).as.variable)
#define expression_as_assignment(expression) ((expression).as.assignment)

Expression* expression_allocate(Expression expr);
void expression_print(Expression* expression, int indent);
void expression_print_tree(Expression* expression, int indent);
void expression_free(Expression* expression);

typedef uint8_t StatementKind;
enum
{
    StatementKind_Invalid,

    StatementKind_Expression,
    StatementKind_Variable_Declaration,
    StatementKind_Print,
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
        Expression* _returned;
        Statement* _block;
        struct { ObjectString* identifier; Expression* rhs; } variable_declaration;
    };
};

#define statement_make_expression(expression) ((Statement) {.kind = StatementKind_Expression, .as = {.expression = (expression)}})
#define statement_make_variable_declaration(name, expresion) ((Statement) {.kind = StatementKind_Variable_Declaration, .as = {.variable_declaration = {.name = name, .expression = (expression)}}})

Statement* statement_allocate(Statement statement);
void statement_print(Statement* statement, int indent);

typedef struct
{
    Statement* items;
    uint32_t count;
    uint32_t capacity;
} ArrayStatement;


ArrayStatement* array_statement_allocate();
uint32_t array_statement_insert(ArrayStatement* statements, Statement statement);
void array_statement_free(ArrayStatement* statements);


//
// Scope
//

#define UINT8_COUNT (UINT8_MAX + 1)
#define INITIALIZED -1


typedef struct {
    Token token;       // has to be a identifier
    int scope_depth;
} Local;

typedef struct {
    Local* items;
    int count;
    int capacity;
} StackLocal;

void  StackLocal_init(StackLocal* locals, int capacity);
Local StackLocal_push(StackLocal* locals, Token token, int scope_depth);
Local StackLocal_pop(StackLocal* local);
int   StackLocal_get_local_index_by_token(StackLocal* locals, Token* token, Local** local_out);
bool  StackLocal_is_full(StackLocal* locals);

typedef struct {
    StackLocal locals;
    int depth;
} Scope; // ScopeEmulator 

void scope_init(Scope* scope);

//
// Parser
//

typedef enum
{
    Operation_Min,

    Operation_Assignment,                  // =
    Operation_Or,                          // or
    Operation_And,                         // and
    Operation_Equality,                    // == =/=
    Operation_Comparison,                  // < > <= >=
    Operation_Addition_And_Subtraction,    // + -
    Operation_Multiplication_And_Division, // * /
    Operation_Negate,                      // Unary:  -
    Operation_Exponentiation,              // ^   ex: -2^2 = -1 * 2^2 = -4
    Operation_Not,                         // Unary: ka
    Operation_Grouping_Call_And_Get,       // . (

    Operation_Max
} OrderOfOperation;

typedef struct
{
    Token token_current;
    Token token_previous;
    Lexer* lexer;
    Scope* scope_current;
    bool had_error;
    bool panic_mode;

    int interpolation_count_nesting;
    int interpolation_count_value_pushed;
} Parser;

#define parser_init(parser, source_code) parser_initialize(parser, source_code, NULL)

void parser_initialize(Parser* parser, const char* source_code, Lexer* lexer);
ArrayStatement* parser_parse(Parser* parser);

//
// Line Number
//

typedef struct
{
    int* items;
    int count;
    int capacity;
} ArrayLineNumber;

void array_line_init(ArrayLineNumber* lines);
int array_line_insert(ArrayLineNumber* lines, int line);
int array_line_insert_3x(ArrayLineNumber* lines, int line);
void array_line_free(ArrayLineNumber* lines);

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

    OpCode_Print,
    OpCode_Jump_If_False,
    OpCode_Jump,
    OpCode_Pop,
    OpCode_Define_Global, // var x;
    OpCode_Read_Global,   // print x;
    OpCode_Assign_Global, // x = 7;
    OpCode_Read_Local,   // print x;
    OpCode_Assign_Local,  // x = 7;

    OpCode_Return
};

typedef struct
{
    uint8_t* items;
    int count;
    int capacity;
} ArrayInstruction;

void array_instruction_init(ArrayInstruction* instructions);
int array_instruction_insert(ArrayInstruction* instructions, uint8_t item);
int array_instruction_insert_u24(ArrayInstruction* instructions, uint8_t byte1, uint8_t byte2, uint8_t byte3);
void array_instruction_free(ArrayInstruction* instructions);

//
// Bytecode
//

// Bytecode is a series of instructions. Eventually, we'll store
// some other data along with instructions, so ...
//
typedef struct
{
    ArrayInstruction instructions;
    ArrayValue values;
    ArrayLineNumber lines;
} Bytecode;

extern Bytecode g_bytecode;

#define bytecode_emit_instruction_1byte(opcode, line) bytecode_insert_instruction_1byte(&g_bytecode, opcode, line)
#define bytecode_emit_instruction_2bytes(opcode, operand, line) bytecode_insert_instruction_2bytes(&g_bytecode, opcode, operand, line)
#define bytecode_emit_constant(value, line) bytecode_insert_instruction_constant(&g_bytecode, value, line)
#define bytecode_emit_instruction_jump(opcode, line) bytecode_insert_instruction_jump(&g_bytecode, opcode, line)
#define Bytecode_PatchInstructionJump(operand_index) bytecode_patch_instruction_jump(&g_bytecode, operand_index)
#define bytecode_emit_value(value) array_value_insert(&g_bytecode.values, value)

// TODO: add function bytecode_jump(Bytecode b, int offset_amount);
// TODO: add function bytecode_instruction_patch_2byte(Bytecode b, int offset, uint8_t byte1, uint8_t byte2);
void bytecode_init(Bytecode* bytecode);
int bytecode_insert_instruction_1byte(Bytecode* bytecode, OpCode opcode, int line_number);
int bytecode_insert_instruction_2bytes(Bytecode* bytecode, OpCode opcode, uint8_t operand, int line_number);
int bytecode_insert_instruction_constant(Bytecode* bytecode, Value value, int line_number);
int bytecode_insert_instruction_jump(Bytecode* bytecode, OpCode opcode, int line);
bool bytecode_patch_instruction_jump(Bytecode* bytecode, int operand_index);
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

// #define DEBUG_TRACE_EXECUTION

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
    //
    Object* objects;

    // Stores global variables 
    //
    HashTable global_database; // TODO: change name to global_database

    // Stores all unique strings allocated during runtime
    //
    HashTable string_database; // TODO: change name to stting_database

    // TODO: add a varible to track total bytes allocated
} VirtualMachine;

extern VirtualMachine g_vm;

#define vm_init() virtual_machine_init(&g_vm)
#define vm_interpret(bytecode) virtual_machine_interpret(&g_vm, bytecode)
#define vm_free() virtual_machine_free(&g_vm)

void virtual_machine_init(VirtualMachine* vm);
InterpreterResult virtual_machine_interpret(VirtualMachine* vm, Bytecode* bytecode);
void virtual_machine_free(VirtualMachine* vm);


#endif