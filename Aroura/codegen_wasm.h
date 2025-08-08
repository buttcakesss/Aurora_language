#ifndef CODEGEN_WASM_H
#define CODEGEN_WASM_H

#include "ast.h"
#include <stdio.h>
#include <stdbool.h>

// WASM value types
typedef enum {
    WASM_I32,
    WASM_I64,
    WASM_F32,
    WASM_F64,
    WASM_VOID
} WasmType;

// Symbol table entry
typedef struct Symbol {
    char* name;
    WasmType type;
    int local_index;
    bool is_function;
    bool is_const;
    struct Symbol* next;
} Symbol;

// Symbol table (linked list)
typedef struct SymbolTable {
    Symbol* head;
    struct SymbolTable* parent;
    int next_local_index;
} SymbolTable;

// String buffer for WAT output
typedef struct {
    char* data;
    size_t size;
    size_t capacity;
} StringBuffer;

// Code generator context
typedef struct {
    StringBuffer* output;
    StringBuffer* functions;
    StringBuffer* globals;
    StringBuffer* data_section;
    SymbolTable* symbols;
    int next_global_index;
    int next_string_index;
    int indent_level;
    bool in_function;
    WasmType current_return_type;
} CodegenContext;

// Function prototypes
CodegenContext* codegen_create(void);
void codegen_destroy(CodegenContext* ctx);
char* codegen_generate(CodegenContext* ctx, Program* program);

// Symbol table functions
SymbolTable* symbol_table_create(SymbolTable* parent);
void symbol_table_destroy(SymbolTable* table);
Symbol* symbol_table_add(SymbolTable* table, const char* name, WasmType type, bool is_function, bool is_const);
Symbol* symbol_table_lookup(SymbolTable* table, const char* name);

// String buffer functions
StringBuffer* string_buffer_create(void);
void string_buffer_destroy(StringBuffer* buf);
void string_buffer_append(StringBuffer* buf, const char* str);
void string_buffer_appendf(StringBuffer* buf, const char* format, ...);
char* string_buffer_to_string(StringBuffer* buf);

// Type conversion
WasmType aurora_type_to_wasm(const char* type_name);
const char* wasm_type_to_string(WasmType type);

// Code generation functions
void codegen_program(CodegenContext* ctx, Program* program);
void codegen_statement(CodegenContext* ctx, Statement* stmt);
void codegen_expression(CodegenContext* ctx, Expression* expr);
void codegen_function(CodegenContext* ctx, Statement* func);
void codegen_binary_op(CodegenContext* ctx, Expression* expr);
void codegen_call(CodegenContext* ctx, Expression* expr);

// Helper functions
void codegen_indent(CodegenContext* ctx);
void codegen_emit(CodegenContext* ctx, const char* format, ...);
void codegen_emit_line(CodegenContext* ctx, const char* format, ...);

#endif // CODEGEN_WASM_H