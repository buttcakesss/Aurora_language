#include "codegen_wasm.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

// Create string buffer
StringBuffer* string_buffer_create(void) {
    StringBuffer* buf = (StringBuffer*)malloc(sizeof(StringBuffer));
    if (!buf) return NULL;
    
    buf->capacity = 1024;
    buf->size = 0;
    buf->data = (char*)malloc(buf->capacity);
    buf->data[0] = '\0';
    
    return buf;
}

// Destroy string buffer
void string_buffer_destroy(StringBuffer* buf) {
    if (buf) {
        free(buf->data);
        free(buf);
    }
}

// Append string to buffer
void string_buffer_append(StringBuffer* buf, const char* str) {
    if (!buf || !str) return;
    
    size_t len = strlen(str);
    size_t needed = buf->size + len + 1;
    
    if (needed > buf->capacity) {
        while (needed > buf->capacity) {
            buf->capacity *= 2;
        }
        buf->data = (char*)realloc(buf->data, buf->capacity);
    }
    
    strcpy(buf->data + buf->size, str);
    buf->size += len;
}

// Append formatted string to buffer
void string_buffer_appendf(StringBuffer* buf, const char* format, ...) {
    char temp[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(temp, sizeof(temp), format, args);
    va_end(args);
    string_buffer_append(buf, temp);
}

// Convert buffer to string
char* string_buffer_to_string(StringBuffer* buf) {
    if (!buf) return NULL;
    return strdup(buf->data);
}

// Create symbol table
SymbolTable* symbol_table_create(SymbolTable* parent) {
    SymbolTable* table = (SymbolTable*)calloc(1, sizeof(SymbolTable));
    if (!table) return NULL;
    
    table->parent = parent;
    table->next_local_index = parent ? parent->next_local_index : 0;
    
    return table;
}

// Destroy symbol table
void symbol_table_destroy(SymbolTable* table) {
    if (!table) return;
    
    Symbol* current = table->head;
    while (current) {
        Symbol* next = current->next;
        free(current->name);
        free(current);
        current = next;
    }
    
    free(table);
}

// Add symbol to table
Symbol* symbol_table_add(SymbolTable* table, const char* name, WasmType type, bool is_function, bool is_const) {
    Symbol* sym = (Symbol*)malloc(sizeof(Symbol));
    if (!sym) return NULL;
    
    sym->name = strdup(name);
    sym->type = type;
    sym->is_function = is_function;
    sym->is_const = is_const;
    
    if (!is_function) {
        sym->local_index = table->next_local_index++;
    } else {
        sym->local_index = -1;
    }
    
    sym->next = table->head;
    table->head = sym;
    
    return sym;
}

// Lookup symbol in table
Symbol* symbol_table_lookup(SymbolTable* table, const char* name) {
    while (table) {
        Symbol* current = table->head;
        while (current) {
            if (strcmp(current->name, name) == 0) {
                return current;
            }
            current = current->next;
        }
        table = table->parent;
    }
    return NULL;
}

// Convert Aurora type to WASM type
WasmType aurora_type_to_wasm(const char* type_name) {
    if (!type_name) return WASM_I32;
    
    if (strcmp(type_name, "int") == 0) return WASM_I32;
    if (strcmp(type_name, "float") == 0) return WASM_F32;
    if (strcmp(type_name, "bool") == 0) return WASM_I32;
    if (strcmp(type_name, "string") == 0) return WASM_I32; // String as pointer
    if (strcmp(type_name, "void") == 0) return WASM_VOID;
    
    return WASM_I32; // Default
}

// Convert WASM type to string
const char* wasm_type_to_string(WasmType type) {
    switch (type) {
        case WASM_I32: return "i32";
        case WASM_I64: return "i64";
        case WASM_F32: return "f32";
        case WASM_F64: return "f64";
        case WASM_VOID: return "";
        default: return "i32";
    }
}

// Create code generator context
CodegenContext* codegen_create(void) {
    CodegenContext* ctx = (CodegenContext*)calloc(1, sizeof(CodegenContext));
    if (!ctx) return NULL;
    
    ctx->output = string_buffer_create();
    ctx->functions = string_buffer_create();
    ctx->globals = string_buffer_create();
    ctx->data_section = string_buffer_create();
    ctx->symbols = symbol_table_create(NULL);
    ctx->next_global_index = 0;
    ctx->next_string_index = 0;
    ctx->indent_level = 0;
    ctx->in_function = false;
    ctx->current_return_type = WASM_VOID;
    
    return ctx;
}

// Destroy code generator context
void codegen_destroy(CodegenContext* ctx) {
    if (!ctx) return;
    
    string_buffer_destroy(ctx->output);
    string_buffer_destroy(ctx->functions);
    string_buffer_destroy(ctx->globals);
    string_buffer_destroy(ctx->data_section);
    symbol_table_destroy(ctx->symbols);
    free(ctx);
}

// Emit indentation
void codegen_indent(CodegenContext* ctx) {
    for (int i = 0; i < ctx->indent_level; i++) {
        string_buffer_append(ctx->output, "  ");
    }
}

// Emit code
void codegen_emit(CodegenContext* ctx, const char* format, ...) {
    char temp[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(temp, sizeof(temp), format, args);
    va_end(args);
    
    StringBuffer* target = ctx->in_function ? ctx->functions : ctx->output;
    string_buffer_append(target, temp);
}

// Emit line of code with indentation
void codegen_emit_line(CodegenContext* ctx, const char* format, ...) {
    char temp[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(temp, sizeof(temp), format, args);
    va_end(args);
    
    codegen_indent(ctx);
    codegen_emit(ctx, "%s\n", temp);
}

// Generate code for binary operation
void codegen_binary_op(CodegenContext* ctx, Expression* expr) {
    // Generate left operand
    codegen_expression(ctx, expr->data.binary.left);
    
    // Generate right operand
    codegen_expression(ctx, expr->data.binary.right);
    
    // Generate operation
    switch (expr->data.binary.op) {
        case TOK_PLUS:
            codegen_emit_line(ctx, "i32.add");
            break;
        case TOK_MINUS:
            codegen_emit_line(ctx, "i32.sub");
            break;
        case TOK_STAR:
            codegen_emit_line(ctx, "i32.mul");
            break;
        case TOK_SLASH:
            codegen_emit_line(ctx, "i32.div_s");
            break;
        case TOK_MOD:
            codegen_emit_line(ctx, "i32.rem_s");
            break;
        case TOK_EQEQ:
            codegen_emit_line(ctx, "i32.eq");
            break;
        case TOK_NEQ:
            codegen_emit_line(ctx, "i32.ne");
            break;
        case TOK_LT:
            codegen_emit_line(ctx, "i32.lt_s");
            break;
        case TOK_GT:
            codegen_emit_line(ctx, "i32.gt_s");
            break;
        case TOK_LTE:
            codegen_emit_line(ctx, "i32.le_s");
            break;
        case TOK_GTE:
            codegen_emit_line(ctx, "i32.ge_s");
            break;
        default:
            codegen_emit_line(ctx, ";; Unknown binary operator");
            break;
    }
}

// Generate code for function call
void codegen_call(CodegenContext* ctx, Expression* expr) {
    // Special handling for print function
    if (strcmp(expr->data.call.name, "print") == 0) {
        // For now, just print integers
        if (expr->data.call.args && expr->data.call.args->size > 0) {
            codegen_expression(ctx, expr->data.call.args->items[0]);
            codegen_emit_line(ctx, "call $print_i32");
        }
        return;
    }
    
    // Generate arguments
    if (expr->data.call.args) {
        for (size_t i = 0; i < expr->data.call.args->size; i++) {
            codegen_expression(ctx, expr->data.call.args->items[i]);
        }
    }
    
    // Generate call
    codegen_emit_line(ctx, "call $%s", expr->data.call.name);
}

// Generate code for expression
void codegen_expression(CodegenContext* ctx, Expression* expr) {
    if (!expr) return;
    
    switch (expr->type) {
        case AST_INT_LITERAL:
            codegen_emit_line(ctx, "i32.const %d", expr->data.literal.value.int_val);
            break;
            
        case AST_FLOAT_LITERAL:
            codegen_emit_line(ctx, "f32.const %f", expr->data.literal.value.float_val);
            break;
            
        case AST_BOOL_LITERAL:
            codegen_emit_line(ctx, "i32.const %d", expr->data.literal.value.bool_val ? 1 : 0);
            break;
            
        case AST_STRING_LITERAL:
            // For now, just push a string index
            codegen_emit_line(ctx, "i32.const %d ;; string: \"%s\"", 
                            ctx->next_string_index++, 
                            expr->data.literal.value.string_val);
            break;
            
        case AST_VARIABLE:
            {
                Symbol* sym = symbol_table_lookup(ctx->symbols, expr->data.variable.name);
                if (sym) {
                    codegen_emit_line(ctx, "local.get $%s", expr->data.variable.name);
                } else {
                    codegen_emit_line(ctx, ";; Unknown variable: %s", expr->data.variable.name);
                }
            }
            break;
            
        case AST_BINARY_OP:
            codegen_binary_op(ctx, expr);
            break;
            
        case AST_UNARY_OP:
            codegen_expression(ctx, expr->data.unary.operand);
            if (expr->data.unary.op == TOK_MINUS) {
                codegen_emit_line(ctx, "i32.const -1");
                codegen_emit_line(ctx, "i32.mul");
            }
            break;
            
        case AST_CALL:
            codegen_call(ctx, expr);
            break;
            
        default:
            codegen_emit_line(ctx, ";; Unknown expression type");
            break;
    }
}

// Generate code for statement
void codegen_statement(CodegenContext* ctx, Statement* stmt) {
    if (!stmt) return;
    
    switch (stmt->type) {
        case AST_LET_STMT:
            {
                WasmType type = WASM_I32;
                if (stmt->data.let_stmt.type_annot) {
                    type = aurora_type_to_wasm(stmt->data.let_stmt.type_annot->name);
                }
                
                // Add to symbol table
                symbol_table_add(ctx->symbols, stmt->data.let_stmt.name, type, false, stmt->data.let_stmt.is_const);
                
                // Generate initializer
                if (stmt->data.let_stmt.value) {
                    codegen_expression(ctx, stmt->data.let_stmt.value);
                    codegen_emit_line(ctx, "local.set $%s", stmt->data.let_stmt.name);
                }
            }
            break;
            
        case AST_ASSIGN_STMT:
            codegen_expression(ctx, stmt->data.assign.value);
            codegen_emit_line(ctx, "local.set $%s", stmt->data.assign.name);
            break;
            
        case AST_FUNC_DEF:
            codegen_function(ctx, stmt);
            break;
            
        case AST_RETURN_STMT:
            if (stmt->data.return_stmt.value) {
                codegen_expression(ctx, stmt->data.return_stmt.value);
            }
            codegen_emit_line(ctx, "return");
            break;
            
        case AST_IF_STMT:
            codegen_expression(ctx, stmt->data.if_stmt.condition);
            codegen_emit_line(ctx, "if");
            ctx->indent_level++;
            
            // Then branch
            if (stmt->data.if_stmt.then_branch) {
                for (size_t i = 0; i < stmt->data.if_stmt.then_branch->size; i++) {
                    codegen_statement(ctx, stmt->data.if_stmt.then_branch->items[i]);
                }
            }
            
            // Else branch
            if (stmt->data.if_stmt.else_branch && stmt->data.if_stmt.else_branch->size > 0) {
                ctx->indent_level--;
                codegen_emit_line(ctx, "else");
                ctx->indent_level++;
                
                for (size_t i = 0; i < stmt->data.if_stmt.else_branch->size; i++) {
                    codegen_statement(ctx, stmt->data.if_stmt.else_branch->items[i]);
                }
            }
            
            ctx->indent_level--;
            codegen_emit_line(ctx, "end");
            break;
            
        case AST_WHILE_STMT:
            codegen_emit_line(ctx, "loop $loop_%d", ctx->next_global_index++);
            ctx->indent_level++;
            
            // Check condition
            codegen_expression(ctx, stmt->data.while_stmt.condition);
            codegen_emit_line(ctx, "i32.eqz");
            codegen_emit_line(ctx, "br_if 1");
            
            // Body
            if (stmt->data.while_stmt.body) {
                for (size_t i = 0; i < stmt->data.while_stmt.body->size; i++) {
                    codegen_statement(ctx, stmt->data.while_stmt.body->items[i]);
                }
            }
            
            codegen_emit_line(ctx, "br 0");
            ctx->indent_level--;
            codegen_emit_line(ctx, "end");
            break;
            
        case AST_FOR_STMT:
            {
                // Create loop variable
                symbol_table_add(ctx->symbols, stmt->data.for_stmt.var, WASM_I32, false, false);
                
                // Initialize loop variable
                codegen_expression(ctx, stmt->data.for_stmt.start);
                codegen_emit_line(ctx, "local.set $%s", stmt->data.for_stmt.var);
                
                codegen_emit_line(ctx, "loop $loop_%d", ctx->next_global_index++);
                ctx->indent_level++;
                
                // Check condition (i < end)
                codegen_emit_line(ctx, "local.get $%s", stmt->data.for_stmt.var);
                codegen_expression(ctx, stmt->data.for_stmt.end);
                codegen_emit_line(ctx, "i32.ge_s");
                codegen_emit_line(ctx, "br_if 1");
                
                // Body
                if (stmt->data.for_stmt.body) {
                    for (size_t i = 0; i < stmt->data.for_stmt.body->size; i++) {
                        codegen_statement(ctx, stmt->data.for_stmt.body->items[i]);
                    }
                }
                
                // Increment loop variable
                codegen_emit_line(ctx, "local.get $%s", stmt->data.for_stmt.var);
                codegen_emit_line(ctx, "i32.const 1");
                codegen_emit_line(ctx, "i32.add");
                codegen_emit_line(ctx, "local.set $%s", stmt->data.for_stmt.var);
                
                codegen_emit_line(ctx, "br 0");
                ctx->indent_level--;
                codegen_emit_line(ctx, "end");
            }
            break;
            
        case AST_EXPR_STMT:
            codegen_expression(ctx, stmt->data.expr_stmt.expr);
            // Drop the result if it's not used
            if (ctx->current_return_type == WASM_VOID) {
                codegen_emit_line(ctx, "drop");
            }
            break;
            
        default:
            codegen_emit_line(ctx, ";; Unknown statement type");
            break;
    }
}

// Generate code for function
void codegen_function(CodegenContext* ctx, Statement* func) {
    // Save state
    bool was_in_function = ctx->in_function;
    SymbolTable* old_symbols = ctx->symbols;
    WasmType old_return_type = ctx->current_return_type;
    
    // Create new scope
    ctx->in_function = true;
    ctx->symbols = symbol_table_create(old_symbols);
    
    // Get return type
    WasmType return_type = WASM_VOID;
    if (func->data.func_def.return_type) {
        return_type = aurora_type_to_wasm(func->data.func_def.return_type->name);
    }
    ctx->current_return_type = return_type;
    
    // Function signature
    codegen_emit_line(ctx, "(func $%s", func->data.func_def.name);
    ctx->indent_level++;
    
    // Parameters
    if (func->data.func_def.params) {
        for (size_t i = 0; i < func->data.func_def.params->size; i++) {
            Parameter* param = func->data.func_def.params->items[i];
            WasmType ptype = aurora_type_to_wasm(param->type->name);
            codegen_emit_line(ctx, "(param $%s %s)", param->name, wasm_type_to_string(ptype));
            symbol_table_add(ctx->symbols, param->name, ptype, false, false);
        }
    }
    
    // Return type
    if (return_type != WASM_VOID) {
        codegen_emit_line(ctx, "(result %s)", wasm_type_to_string(return_type));
    }
    
    // Local variables (collect from function body)
    StringBuffer* locals = string_buffer_create();
    Statement** stmts = func->data.func_def.body->items;
    for (size_t i = 0; i < func->data.func_def.body->size; i++) {
        if (stmts[i]->type == AST_LET_STMT) {
            WasmType type = WASM_I32;
            if (stmts[i]->data.let_stmt.type_annot) {
                type = aurora_type_to_wasm(stmts[i]->data.let_stmt.type_annot->name);
            }
            string_buffer_appendf(locals, "    (local $%s %s)\n", 
                                stmts[i]->data.let_stmt.name, 
                                wasm_type_to_string(type));
        }
    }
    
    // Output locals
    codegen_emit(ctx, "%s", locals->data);
    string_buffer_destroy(locals);
    
    // Function body
    if (func->data.func_def.body) {
        for (size_t i = 0; i < func->data.func_def.body->size; i++) {
            codegen_statement(ctx, func->data.func_def.body->items[i]);
        }
    }
    
    // Add implicit return for void functions
    if (return_type == WASM_VOID) {
        codegen_emit_line(ctx, "return");
    }
    
    ctx->indent_level--;
    codegen_emit_line(ctx, ")");
    
    // Add to symbol table
    symbol_table_add(old_symbols, func->data.func_def.name, return_type, true, false);
    
    // Restore state
    symbol_table_destroy(ctx->symbols);
    ctx->symbols = old_symbols;
    ctx->in_function = was_in_function;
    ctx->current_return_type = old_return_type;
}

// Generate code for program
void codegen_program(CodegenContext* ctx, Program* program) {
    // WAT module header
    string_buffer_append(ctx->output, "(module\n");
    ctx->indent_level = 1;
    
    // Import print function for debugging
    codegen_emit_line(ctx, ";; Runtime imports");
    codegen_emit_line(ctx, "(import \"env\" \"print_i32\" (func $print_i32 (param i32)))");
    codegen_emit_line(ctx, "");
    
    // Process all statements
    if (program->statements) {
        for (size_t i = 0; i < program->statements->size; i++) {
            Statement* stmt = program->statements->items[i];
            
            // Functions go to function section
            if (stmt->type == AST_FUNC_DEF) {
                codegen_function(ctx, stmt);
            } else {
                // Global statements go to start function
                // For now, skip global non-function statements
                codegen_emit_line(ctx, ";; Global statement (not yet supported)");
            }
        }
    }
    
    // Add main/start function if needed
    Symbol* main_sym = symbol_table_lookup(ctx->symbols, "main");
    if (main_sym) {
        codegen_emit_line(ctx, "");
        codegen_emit_line(ctx, ";; Export main function");
        codegen_emit_line(ctx, "(export \"main\" (func $main))");
    }
    
    // Add functions to output
    if (ctx->functions->size > 0) {
        codegen_emit_line(ctx, "");
        codegen_emit_line(ctx, ";; Functions");
        string_buffer_append(ctx->output, ctx->functions->data);
    }
    
    // Module footer
    ctx->indent_level = 0;
    string_buffer_append(ctx->output, ")\n");
}

// Main generation function
char* codegen_generate(CodegenContext* ctx, Program* program) {
    codegen_program(ctx, program);
    return string_buffer_to_string(ctx->output);
}