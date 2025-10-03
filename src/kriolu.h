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

#include "time.h"
#include "../codebase/codebase.h"

// 
// Debug defines
// 

#define DEBUG_LOG_PARSER
#define DEBUG_TRACE_INSTRUCTION true
#define DEBUG_GC_TRACE
#define DEBUG_GC_STRESS
// #define DEBUG_TRACE_EXECUTION
// #define DEBUG_COMPILER_BYTECODE

//
// Token
//

typedef enum {
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
    Token_Sai,
    Token_Salta,
    Token_Keli,    // THIS
    Token_Mimoria, // VARIABLE
    Token_Timenti,
    Token_Pa,
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

typedef struct {
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
typedef struct {
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
void string_init(String* string, const char* characters, int length);
String string_make(const char* characters, int length);
String string_make_from_format(const char* format, ...);
String string_copy(const char* characters, int length);
String string_copy_from_other(String other);
String string_concatenate(String a, String b);
uint32_t string_hash(String string);
bool string_equal(String a, String b);
void string_free(String* string);

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
enum {
    OpCode_Invalid,

    OpCode_Stack_Push_Literal,
    OpCode_Stack_Push_Literal_Long,
    OpCode_Stack_Push_Literal_Nil,
    OpCode_Stack_Push_Literal_True,
    OpCode_Stack_Push_Literal_False,
    OpCode_Stack_Push_Closure,
    OpCode_Stack_Push_Closure_Long,
    OpCode_Stack_Pop,
    OpCode_Stack_Copy_From_idx_To_Top,
    OpCode_Stack_Copy_Top_To_Idx,
    OpCode_Move_Value_To_Heap,
    OpCode_Interpolation,
    OpCode_Negation,
    OpCode_Not,
    OpCode_Add,
    OpCode_Subtract,
    OpCode_Multiply,
    OpCode_Divide,
    OpCode_Exponentiation,  // TODO: rename to 'OpCode_Exponentiate'
    // TODO: add OpCode_Modulus
    OpCode_Equal_To,
    OpCode_Greater_Than,
    OpCode_Less_Than,

    OpCode_Print,
    OpCode_Jump_If_False,
    OpCode_Jump,
    OpCode_Define_Global,
    OpCode_Read_Global,
    OpCode_Assign_Global,

    OpCode_Copy_From_Heap_To_Stack,
    OpCode_Copy_From_Stack_To_Heap,
    OpCode_Loop,
    OpCode_Call_Function,

    OpCode_Return
};

typedef struct
{
    uint8_t* items;
    int count;
    int capacity;
} ArrayInstruction;

void array_instruction_init(ArrayInstruction* instructions);
int  array_instruction_insert(ArrayInstruction* instructions, uint8_t item);
int  array_instruction_insert_u24(ArrayInstruction* instructions, uint8_t byte1, uint8_t byte2, uint8_t byte3);
void array_instruction_free(ArrayInstruction* instructions);

//
// Value
//

typedef struct Object Object;

typedef enum {
    Value_Runtime_Error,

    Value_Boolean,
    Value_Nil,
    Value_Number,
    Value_Object,

    Value_Count
} ValueKind;

typedef struct {
    ValueKind kind;
    union
    {
        bool boolean;
        double number;
        Object* object;
    } as;
} Value;

typedef struct {
    Value* items;
    int count;
    int capacity;
} ArrayValue;

#define value_make_Runtime_Error() ((Value){.kind = Value_Runtime_Error, .as = {.number = 0}})
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
#define value_as_function_object(value) ((ObjectFunction *)value_as_object(value))
#define value_as_function_native(value) ((ObjectFunctionNative*)value_as_object(value))
#define value_as_closure(value) ((ObjectClosure*)value_as_object(value))

#define value_is_runtime_error(value) ((value).kind == Value_Runtime_Error)
#define value_is_boolean(value) ((value).kind == Value_Boolean)
#define value_is_number(value) ((value).kind == Value_Number)
#define value_is_object(value) ((value).kind == Value_Object)
#define value_is_nil(value) ((value).kind == Value_Nil)
#define value_is_string(value) object_validate_kind_from_value(value, ObjectKind_String)
#define value_is_function(value) object_validate_kind_from_value(value, ObjectKind_Function)
#define value_is_function_native(value) object_validate_kind_from_value(value, ObjectKind_Function_Native)
#define value_is_closure(value) object_validate_kind_from_value(value, ObjectKind_Closure)

#define value_get_object_type(value) value_as_object(value)->kind
#define value_get_string_chars(value) (value_as_string(value)->characters)

bool value_negate_logically(Value value);
bool value_is_falsey(Value value);
bool value_is_equal(Value a, Value b);
void value_print(Value value);

void array_value_init(ArrayValue* values);
uint32_t array_value_insert(ArrayValue* values, Value value);
void array_value_free(ArrayValue* values);

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

#define Compiler_CompileInstruction_1Byte(bytecode, opcode, line) Bytecode_insert_instruction_1byte(bytecode, opcode, line, DEBUG_TRACE_INSTRUCTION)
#define Compiler_CompileInstruction_2Bytes(bytecode, opcode, operand, line) Bytecode_insert_instruction_2bytes(bytecode, opcode, operand, line, DEBUG_TRACE_INSTRUCTION)
#define Compiler_CompileInstruction_Constant(bytecode, value, line) Bytecode_insert_instruction_constant(bytecode, value, line, DEBUG_TRACE_INSTRUCTION)
#define Compiler_CompileInstruction_Closure(bytecode, value, line) Bytecode_insert_instruction_closure(bytecode, value, line, DEBUG_TRACE_INSTRUCTION)
#define Compiler_CompileInstruction_Jump(bytecode, opcode, line) Bytecode_insert_instruction_jump(bytecode, opcode, line, DEBUG_TRACE_INSTRUCTION)
#define Compiler_CompileInstruction_Loop(bytecode, start_index, line) Bytecode_emit_instruction_loop(bytecode, start_index, line, DEBUG_TRACE_INSTRUCTION)
#define Compiler_CompileValue(bytecode, value) array_value_insert(&bytecode->values, value)
#define Compiler_PatchInstructionJump(bytecode, operand_index) Bytecode_patch_instruction_jump(bytecode, operand_index, DEBUG_TRACE_INSTRUCTION)

void Bytecode_init(Bytecode* bytecode);
int  Bytecode_insert_instruction_1byte(Bytecode* bytecode, OpCode opcode, int line_number, bool debug_trace_on);
int  Bytecode_insert_instruction_2bytes(Bytecode* bytecode, OpCode opcode, uint8_t operand, int line_number, bool debug_trace_on);
int  Bytecode_insert_instruction_constant(Bytecode* bytecode, Value value, int line_number, bool debug_trace_on);
void Bytecode_insert_instruction_closure(Bytecode* bytecode, Value value, int line_number, bool debug_trace_on);
int  Bytecode_insert_instruction_jump(Bytecode* bytecode, OpCode opcode, int line, bool debug_trace_on);
void Bytecode_emit_instruction_loop(Bytecode* bytecode, int jump_to_index, int line_number, bool debug_trace_on);
bool Bytecode_patch_instruction_jump(Bytecode* bytecode, int operand_index, bool debug_trace_on);
void Bytecode_disassemble_header(char* title_name);
void Bytecode_disassemble(Bytecode* bytecode, const char* name);
int  Bytecode_disassemble_instruction(Bytecode* bytecode, int offset);
void Bytecode_free(Bytecode* bytecode);

//
// Object / Runtime Values
//

typedef struct VirtualMachine VirtualMachine;
typedef struct HashTable HashTable;

typedef enum {
    ObjectKind_Invalid,

    ObjectKind_String,
    ObjectKind_Function,
    ObjectKind_Function_Native,
    ObjectKind_Closure,
    ObjectKind_Heap_Value,

    ObjectKind_Count
} ObjectKind;

static const char* ObjectKind_text[] = {
    [ObjectKind_String]          = "String",
    [ObjectKind_Function]        = "Function",
    [ObjectKind_Function_Native] = "Function Native",
    [ObjectKind_Closure]         = "Closure",
    [ObjectKind_Heap_Value]      = "Heap Value",
};

struct Object {
    ObjectKind  kind;
    bool        is_marked;
    Object     *next;
};

typedef struct {
    Object object;

    char* characters;
    int length;
    uint32_t hash;
} ObjectString;

typedef struct {
    Object object;

    Bytecode bytecode;
    ObjectString* name;
    int arity; // Number of parameters
    int variable_dependencies_count;
} ObjectFunction;

typedef Value FunctionNative(VirtualMachine* vm, int argument_count, Value* arguments);
typedef struct {
    Object object;

    FunctionNative* function;
    int arity;
} ObjectFunctionNative;

typedef struct ObjectValue ObjectValue;
struct ObjectValue {
    Object object;

    Value* value_address; 
    Value value;
    ObjectValue* next;
};

typedef struct {
    ObjectValue** items;
    int count;
} ArrayObjectValue;

typedef struct {
    Object object;

    ObjectFunction* function;
    ArrayObjectValue heap_values;
} ObjectClosure;

typedef enum {
    AllocateTask_Initialize         = (1 << 0),     
    AllocateTask_Intern             = (1 << 1),       
    AllocateTask_Copy_String        = (1 << 2),    
    AllocateTask_Check_If_Interned  = (1 << 3) 
} AllocateTask;

typedef struct {
    AllocateTask task;
    HashTable   *table;    // INTERNING
    String       string;   // Initialization 
    uint32_t     hash;     // Initialization
    Object     **first;    // Initialization
} AllocateParams;

#define ObjectString_Allocate(...) \
    ObjectString_allocate((AllocateParams){__VA_ARGS__})

Object* Object_allocate(ObjectKind kind, size_t size, Object** object_head);
void Object_init(Object* object, ObjectKind kind, Object** object_head);
void Object_print(Object* object);
void Object_free(Object* object);

ObjectString* ObjectString_allocate(AllocateParams params);
ObjectFunction* ObjectFunction_allocate(ObjectString* function_name, Object** object_head);
ObjectValue* ObjectValue_allocate(Object** object_head, Value* value_address);
ObjectClosure* ObjectClosure_allocate(ObjectFunction* function, Object** object_head);
ObjectFunctionNative* ObjectFunctionNative_allocate(FunctionNative* function, Object** object_head, int arity);

// inline bool object_validate_kind_from_value(Value value, ObjectKind object_kind);
// inline bool object_validate_kind_from_value(Value value, ObjectKind object_kind) {
//     return (value_is_object(value) && value_as_object(value)->kind == object_kind);
// }

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

typedef struct {
    ObjectString* key;
    Value value;
} Entry;

struct HashTable {
    Entry* items;
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
    ExpressionKind_Variable,
    ExpressionKind_Not,
    ExpressionKind_And,
    ExpressionKind_Or
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
#define expression_make_variable(variable_name) ((Expression) {.kind = ExpressionKind_Variable, .as = {.variable = (variable_name)}})
#define expression_make_assignment(variable_name, expression) ((Expression) {.kind = ExpressionKind_Assignment, .as = { .assignment = { .lhs = (variable_name), .rhs = (expression) } }})
#define expression_make_and(left_operand, right_operand) ((Expression) {.kind = ExpressionKind_And, .as = { .binary = { .left = (left_operand), .right = (right_operand) }}})
#define expression_make_or(left_operand, right_operand) ((Expression) {.kind = ExpressionKind_Or, .as = { .binary = { .left = (left_operand), .right = (right_operand) }}})

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
#define expression_as_and(expression) ((expression).as.binary)
#define expression_as_or(expression) ((expression).as.binary)

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
    StatementKind_Timenti,
    StatementKind_Pa,
    StatementKind_Sai,
    StatementKind_Salta,
    StatementKind_Funson,

    StatementKind_Max
};

typedef struct ArrayStatement ArrayStatement;
typedef struct Statement Statement;
struct Statement
{
    StatementKind kind;
    union
    {
        Expression* expression;
        Expression* _return; // TODO: rename to 'divovi'
        Expression* print;   // TODO: rename to 'imprimi'
        ArrayStatement* bloco;
        struct { ObjectString* identifier; Expression* rhs; } variable_declaration;
        struct { Expression* condition; Statement* then_block; Statement* else_block; } _if;
        struct { Expression* condition; Statement* body; } timenti;
        struct { Statement* initializer; Expression* condition; Expression* increment; Statement* body; } pa;
    };
};

#define statement_make_expression(expression) ((Statement) {.kind = StatementKind_Expression, .expression = (expression)})
#define statement_make_variable_declaration(name, expresion) ((Statement) {.kind = StatementKind_Variable_Declaration, .variable_declaration = {.name = name, .expression = (expression)}})
#define statement_make_if(condition, then_block, else_block) ((Statement) {.kind = StatementKind_Si, ._if = {.condition = (condition), .then_block = (then_block), .else_block = (else_block)}})

Statement* statement_allocate(Statement statement);
void statement_print(Statement* statement, int indent);

struct ArrayStatement
{
    Statement* items;
    uint32_t count;
    uint32_t capacity;
};

ArrayStatement* array_statement_allocate();
uint32_t array_statement_insert(ArrayStatement* statements, Statement statement);
void array_statement_free(ArrayStatement* statements);

//
// Local
//

#define UINT8_COUNT (UINT8_MAX + 1)
#define INITIALIZED -1

typedef enum {
    LocalAction_Default,
    LocalAction_Move_Heap
} LocalAction;

typedef struct {
    Token token;       // has to be a identifier
    int scope_depth;
    LocalAction action;
} Local;

typedef struct {
    Local items[UINT8_COUNT];
    int top;
    int capacity;
} StackLocal;

void   StackLocal_init(StackLocal* locals);
Local  StackLocal_push(StackLocal* locals, Token token, int scope_depth, LocalAction action);
Local  StackLocal_pop(StackLocal* locals);
Local* StackLocal_peek(StackLocal* locals, int offset);
int    StackLocal_get_local_index_by_token(StackLocal* locals, Token* token, Local** local_out);
bool   StackLocal_is_full(StackLocal* locals);
bool   StackLocal_is_empty(StackLocal* locals);
bool   StackLocal_initialize_local(StackLocal* locals, int depth, int offset);

// 
// Function
// 

typedef enum {
    FunctionKind_Script,
    FunctionKind_Function
} FunctionKind;

typedef enum {
    LocalLocation_In_Parent_Heap_Values = 0,
    LocalLocation_In_Parent_Stack
} LocalLocation;

typedef struct {
    uint8_t index;
    LocalLocation location;
} LocalMetadata;

typedef struct {
    LocalMetadata items[UINT8_COUNT];
    int count;
} ArrayLocalMetadata;

void ArrayLocalMetadata_init(ArrayLocalMetadata* local_metadata, int count);
int  ArrayLocalMetadata_add(ArrayLocalMetadata* local_metadata, uint8_t index, LocalLocation location, int* variable_dependencies_count);

typedef struct Function Function;
struct Function {
    LinkedList(Function) next;

    FunctionKind function_kind;
    ObjectFunction* object;                     // NOTE: Runtime function data structure
    StackLocal locals;                          // NOTE: Variables declared inside a function; Emulates runtime StackValue 
    ArrayLocalMetadata variable_dependencies;   // NOTE: Varibale accessed that belongs to a parent function
    int depth;                                  // NOTE: Scope depth
};

void Function_init(Function* function, FunctionKind function_kind, Function** function_current, ObjectString* function_name, Object** object_head);
ObjectFunction* Function_end(Function* function, Function** Function_current, bool parser_has_error, int line_number);
int Function_resolve_variable_dependencies(Function* function, Token* name, Local** ret_local);

typedef struct StackFunction StackFunction;
struct StackFunction {
    int top;
    Function* items[UINT8_COUNT];
};

void      StackFunction_init(StackFunction* stack_function);
Function* StackFunction_push(StackFunction* stack_function, Function* function);
Function* StackFunction_pop(StackFunction* stack_function);
Function* StackFunction_peek(StackFunction* stack_function, int offset);
bool      StackFunction_is_empty(StackFunction* stack_function);
bool      StackFunction_is_full(StackFunction* stack_function);
void      StackFunction_free(StackFunction* stack_function);

//
// Parser
//

typedef enum {
    OperatorPrecedence_Invalid,

    OperatorPrecedence_Assignment,                  // =
    OperatorPrecedence_Or,                          // or
    OperatorPrecedence_And,                         // and
    OperatorPrecedence_Equality,                    // == =/=
    OperatorPrecedence_Comparison,                  // < > <= >=
    OperatorPrecedence_Addition_And_Subtraction,    // + -
    OperatorPrecedence_Multiplication_And_Division, // * /
    OperatorPrecedence_Negate,                      // Unary:  -
    OperatorPrecedence_Exponentiation,              // ^   ex: -2^2 = -1 * 2^2 = -4
    OperatorPrecedence_Not,                         // Unary: ka
    OperatorPrecedence_Grouping_FunctionCall_And_ObjectGet,       // . (

    OperatorPrecedence_Max
} OperatorPrecedence;

typedef struct {
    OperatorPrecedence precedence;
    bool is_left_associative;
    bool is_right_associative;
} OperatorMetadata;

typedef enum {
    BlockType_None,

    BlockType_Clean,
    BlockType_Loop,
    BlockType_If,
    BlockType_Function
} BlockType;

// typedef struct {
//     bool is_block;
//     BlockType block_type;
// } ParseStatementParams;

#define BREAK_POINT_MAX 200

typedef struct {
    int block_depth;
    int operand_index;
} Breakpoint;

typedef struct {
    Breakpoint items[BREAK_POINT_MAX];
    int top;
} StackBreakpoint;

void StackBreak_init(StackBreakpoint* breakpoints);
Breakpoint StackBreak_push(StackBreakpoint* breakpoints, Breakpoint value);
Breakpoint StackBreak_pop(StackBreakpoint* breakpoints);
Breakpoint StackBreak_peek(StackBreakpoint* breakpoints, int offset);
bool StackBreak_is_empty(StackBreakpoint* breakpoints);
bool StackBreak_is_full(StackBreakpoint* breakpoints);

#define BLOCKS_MAX 200

typedef struct {
    BlockType items[BLOCKS_MAX];
    int top;
} StackBlock;

void StackBlock_init(StackBlock* blocks);
BlockType StackBlock_push(StackBlock* blocks, BlockType value);
BlockType StackBlock_pop(StackBlock* blocks);
BlockType StackBlock_peek(StackBlock* blocks, int offset);
bool StackBlock_is_empty(StackBlock* blocks);
bool StackBlock_is_full(StackBlock* blocks);
int StackBlock_get_top_item_index(StackBlock* blocks);

typedef struct {
    Token token_current;
    Token token_previous;
    Lexer* lexer;
    LinkedList(Function) function;
    HashTable* string_database;
    HashTable table_strings;

    Object** object_head;
    LinkedList(Object) objects;

    int interpolation_count_nesting;
    int interpolation_count_value_pushed;
    int continue_jump_to;
    StackBreakpoint breakpoints;
    StackBlock blocks;

    bool had_error;
    bool panic_mode;
} Parser;

typedef struct {
    Lexer*     lexer;
    HashTable* string_database;
    Object**   object_head;
} ParserInitParams;

#define Parser_Init(parser, source_code, ...) \
    parser_init((parser), (source_code), (ParserInitParams){__VA_ARGS__})

void parser_init(Parser* parser, const char* source_code, ParserInitParams params);
ObjectFunction* parser_parse(Parser* parser, ArrayStatement** return_statements, HashTable* string_database, Object** object_head);

//
// Value Stack
//

#define STACK_MAX 256

typedef struct {
    Value items[STACK_MAX];
    Value* top;
} StackValue;

// StackValue* stack_value_create(void);
void  stack_value_reset(StackValue* stack);
Value stack_value_push(StackValue* stack, Value value);
Value stack_value_pop(StackValue* stack);
Value stack_value_peek(StackValue* stack, int offset);
bool  stack_value_is_full(StackValue* stack);
bool  stack_value_is_empty(StackValue* stack);
void  stack_value_trace(StackValue* stack);
void  stack_free(StackValue* stack);

//
// Memory / Garbage Collector
// 

#define Kilobytes(n) (n * 1024)
#define Megabytes(n) (n * 1024 * 1024)
#define Gigabytes(n) (n * 1024 * 1024 * 1024)

#define Memory_Allocate(Type) \
    Memory_Allocate_Count(Type, 1)

#define Memory_Allocate_Count(type, count)  \
    (type*) Memory_allocate(NULL, 0, sizeof(type) * count)

#define Memory_AllocateArray(type, pointer, old_count, new_count) \
    (type*) Memory_allocate(pointer, sizeof(type) * (old_count), sizeof(type) * (new_count))

#define Memory_Free(type, pointer) \
    Memory_allocate(pointer, sizeof(type), 0)

#define Memory_FreeArray(type, pointer, old_count) \
    Memory_allocate(pointer, sizeof(type) * old_count, 0)

void  Memory_register(size_t* bytes_total_source, VirtualMachine *vm, Parser *parser);
void* Memory_allocate(void* pointer, size_t old_size, size_t new_size);
void  Memory_mark_object_gray(Object* object);
void  Memory_mark_value_gray(Value value);
void  Memory_free_objects();
void  Memory_transaction_push(Value value);
void  Memory_transaction_pop();

//
// Virtual Machine
//

#define FRAME_MAX 64
#define FRAME_STACK_MAX (FRAME_MAX * UINT8_COUNT)

typedef enum
{
    Interpreter_Ok,
    Interpreter_Function_error,
    Interpreter_Runtime_Error
} InterpreterResult;

typedef struct {
    ObjectClosure* closure;
    uint8_t* ip;
    Value* frame_start; // base_pointer
} FunctionCall;

typedef struct {
    FunctionCall items[FRAME_STACK_MAX];
    int top;
} StackFunctionCall;

void StackFunctionCall_reset(StackFunctionCall* function_calls);
FunctionCall* StackFunctionCall_push(StackFunctionCall* function_calls, ObjectClosure* closure, uint8_t* ip, Value* stack_value_top, int argument_count);
FunctionCall* StackFunctionCall_pop(StackFunctionCall* function_calls);
FunctionCall* StackFunctionCall_peek(StackFunctionCall* function_calls, int offset);
bool StackFunctionCall_is_empty(StackFunctionCall* function_calls);
bool StackFunctionCall_is_full(StackFunctionCall* function_calls);

// Forward Declared in line: 343 in Object's section
//
struct VirtualMachine {
    StackFunctionCall function_calls;
    StackValue stack_value; // TODO: rename to stack_values
    LinkedList(Object) objects;

    // Heap Values tracker
    //
    LinkedList(ObjectValue) heap_values;

    // Stores global variables 
    //
    HashTable global_database;

    // Stores all unique string allocated during Compilation and Runtime
    //
    HashTable string_database;

    // TODO: add a varible to track total bytes allocated
};

void VirtualMachine_init(VirtualMachine* vm);
InterpreterResult VirtualMachine_interpret(VirtualMachine* vm, ObjectFunction* script);
void VirtualMachine_free(VirtualMachine* vm);

#endif