#include "ast.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Create program node
Program* program_create(void) {
    Program* prog = (Program*)malloc(sizeof(Program));
    if (!prog) return NULL;
    
    prog->statements = statement_list_create();
    return prog;
}

// Destroy program node
void program_destroy(Program* prog) {
    if (prog) {
        statement_list_destroy(prog->statements);
        free(prog);
    }
}

// Create statement node
Statement* statement_create(ASTNodeType type) {
    Statement* stmt = (Statement*)calloc(1, sizeof(Statement));
    if (!stmt) return NULL;
    
    stmt->type = type;
    return stmt;
}

// Destroy statement node
void statement_destroy(Statement* stmt) {
    if (!stmt) return;
    
    switch (stmt->type) {
        case AST_LET_STMT:
            free(stmt->data.let_stmt.name);
            type_annotation_destroy(stmt->data.let_stmt.type_annot);
            expression_destroy(stmt->data.let_stmt.value);
            break;
            
        case AST_ASSIGN_STMT:
            free(stmt->data.assign.name);
            expression_destroy(stmt->data.assign.value);
            break;
            
        case AST_FUNC_DEF:
            free(stmt->data.func_def.name);
            parameter_list_destroy(stmt->data.func_def.params);
            type_annotation_destroy(stmt->data.func_def.return_type);
            statement_list_destroy(stmt->data.func_def.body);
            break;
            
        case AST_RETURN_STMT:
            expression_destroy(stmt->data.return_stmt.value);
            break;
            
        case AST_IF_STMT:
            expression_destroy(stmt->data.if_stmt.condition);
            statement_list_destroy(stmt->data.if_stmt.then_branch);
            statement_list_destroy(stmt->data.if_stmt.else_branch);
            break;
            
        case AST_WHILE_STMT:
            expression_destroy(stmt->data.while_stmt.condition);
            statement_list_destroy(stmt->data.while_stmt.body);
            break;
            
        case AST_FOR_STMT:
            free(stmt->data.for_stmt.var);
            expression_destroy(stmt->data.for_stmt.start);
            expression_destroy(stmt->data.for_stmt.end);
            statement_list_destroy(stmt->data.for_stmt.body);
            break;
            
        case AST_EXPR_STMT:
            expression_destroy(stmt->data.expr_stmt.expr);
            break;
            
        default:
            break;
    }
    
    free(stmt);
}

// Create expression node
Expression* expression_create(ASTNodeType type) {
    Expression* expr = (Expression*)calloc(1, sizeof(Expression));
    if (!expr) return NULL;
    
    expr->type = type;
    return expr;
}

// Destroy expression node
void expression_destroy(Expression* expr) {
    if (!expr) return;
    
    switch (expr->type) {
        case AST_BINARY_OP:
            expression_destroy(expr->data.binary.left);
            expression_destroy(expr->data.binary.right);
            break;
            
        case AST_UNARY_OP:
            expression_destroy(expr->data.unary.operand);
            break;
            
        case AST_STRING_LITERAL:
            free(expr->data.literal.value.string_val);
            break;
            
        case AST_VARIABLE:
            free(expr->data.variable.name);
            break;
            
        case AST_CALL:
            free(expr->data.call.name);
            expression_list_destroy(expr->data.call.args);
            break;
            
        default:
            break;
    }
    
    free(expr);
}

// Create statement list
StatementList* statement_list_create(void) {
    StatementList* list = (StatementList*)malloc(sizeof(StatementList));
    if (!list) return NULL;
    
    list->capacity = 8;
    list->size = 0;
    list->items = (Statement**)malloc(sizeof(Statement*) * list->capacity);
    
    return list;
}

// Add statement to list
void statement_list_add(StatementList* list, Statement* stmt) {
    if (!list || !stmt) return;
    
    if (list->size >= list->capacity) {
        list->capacity *= 2;
        list->items = (Statement**)realloc(list->items, 
                                          sizeof(Statement*) * list->capacity);
    }
    
    list->items[list->size++] = stmt;
}

// Destroy statement list
void statement_list_destroy(StatementList* list) {
    if (!list) return;
    
    for (size_t i = 0; i < list->size; i++) {
        statement_destroy(list->items[i]);
    }
    
    free(list->items);
    free(list);
}

// Create expression list
ExpressionList* expression_list_create(void) {
    ExpressionList* list = (ExpressionList*)malloc(sizeof(ExpressionList));
    if (!list) return NULL;
    
    list->capacity = 4;
    list->size = 0;
    list->items = (Expression**)malloc(sizeof(Expression*) * list->capacity);
    
    return list;
}

// Add expression to list
void expression_list_add(ExpressionList* list, Expression* expr) {
    if (!list || !expr) return;
    
    if (list->size >= list->capacity) {
        list->capacity *= 2;
        list->items = (Expression**)realloc(list->items, 
                                           sizeof(Expression*) * list->capacity);
    }
    
    list->items[list->size++] = expr;
}

// Destroy expression list
void expression_list_destroy(ExpressionList* list) {
    if (!list) return;
    
    for (size_t i = 0; i < list->size; i++) {
        expression_destroy(list->items[i]);
    }
    
    free(list->items);
    free(list);
}

// Create parameter list
ParameterList* parameter_list_create(void) {
    ParameterList* list = (ParameterList*)malloc(sizeof(ParameterList));
    if (!list) return NULL;
    
    list->capacity = 4;
    list->size = 0;
    list->items = (Parameter**)malloc(sizeof(Parameter*) * list->capacity);
    
    return list;
}

// Add parameter to list
void parameter_list_add(ParameterList* list, Parameter* param) {
    if (!list || !param) return;
    
    if (list->size >= list->capacity) {
        list->capacity *= 2;
        list->items = (Parameter**)realloc(list->items, 
                                          sizeof(Parameter*) * list->capacity);
    }
    
    list->items[list->size++] = param;
}

// Destroy parameter list
void parameter_list_destroy(ParameterList* list) {
    if (!list) return;
    
    for (size_t i = 0; i < list->size; i++) {
        parameter_destroy(list->items[i]);
    }
    
    free(list->items);
    free(list);
}

// Create type annotation
TypeAnnotation* type_annotation_create(const char* name) {
    TypeAnnotation* type = (TypeAnnotation*)malloc(sizeof(TypeAnnotation));
    if (!type) return NULL;
    
    type->name = strdup(name);
    return type;
}

// Destroy type annotation
void type_annotation_destroy(TypeAnnotation* type) {
    if (type) {
        free(type->name);
        free(type);
    }
}

// Create parameter
Parameter* parameter_create(const char* name, TypeAnnotation* type) {
    Parameter* param = (Parameter*)malloc(sizeof(Parameter));
    if (!param) return NULL;
    
    param->name = strdup(name);
    param->type = type;
    return param;
}

// Destroy parameter
void parameter_destroy(Parameter* param) {
    if (param) {
        free(param->name);
        type_annotation_destroy(param->type);
        free(param);
    }
}

// Debug printing functions
static void indent(int level) {
    for (int i = 0; i < level * 2; i++) {
        printf(" ");
    }
}

static void print_expression(Expression* expr, int level);
static void print_statement(Statement* stmt, int level);

static void print_expression(Expression* expr, int level) {
    if (!expr) {
        indent(level);
        printf("(null expression)\n");
        return;
    }
    
    switch (expr->type) {
        case AST_BINARY_OP:
            indent(level);
            printf("BinaryOp:\n");
            indent(level + 1);
            printf("operator: %d\n", expr->data.binary.op);
            indent(level + 1);
            printf("left:\n");
            print_expression(expr->data.binary.left, level + 2);
            indent(level + 1);
            printf("right:\n");
            print_expression(expr->data.binary.right, level + 2);
            break;
            
        case AST_UNARY_OP:
            indent(level);
            printf("UnaryOp:\n");
            indent(level + 1);
            printf("operator: %d\n", expr->data.unary.op);
            indent(level + 1);
            printf("operand:\n");
            print_expression(expr->data.unary.operand, level + 2);
            break;
            
        case AST_INT_LITERAL:
            indent(level);
            printf("IntLiteral: %d\n", expr->data.literal.value.int_val);
            break;
            
        case AST_FLOAT_LITERAL:
            indent(level);
            printf("FloatLiteral: %f\n", expr->data.literal.value.float_val);
            break;
            
        case AST_STRING_LITERAL:
            indent(level);
            printf("StringLiteral: \"%s\"\n", expr->data.literal.value.string_val);
            break;
            
        case AST_BOOL_LITERAL:
            indent(level);
            printf("BoolLiteral: %s\n", expr->data.literal.value.bool_val ? "true" : "false");
            break;
            
        case AST_VARIABLE:
            indent(level);
            printf("Variable: %s\n", expr->data.variable.name);
            break;
            
        case AST_CALL:
            indent(level);
            printf("Call: %s\n", expr->data.call.name);
            if (expr->data.call.args && expr->data.call.args->size > 0) {
                indent(level + 1);
                printf("arguments:\n");
                for (size_t i = 0; i < expr->data.call.args->size; i++) {
                    print_expression(expr->data.call.args->items[i], level + 2);
                }
            }
            break;
            
        default:
            indent(level);
            printf("Unknown expression type: %d\n", expr->type);
    }
}

static void print_statement(Statement* stmt, int level) {
    if (!stmt) {
        indent(level);
        printf("(null statement)\n");
        return;
    }
    
    switch (stmt->type) {
        case AST_LET_STMT:
            indent(level);
            printf("LetStatement:\n");
            indent(level + 1);
            printf("name: %s\n", stmt->data.let_stmt.name);
            indent(level + 1);
            printf("const: %s\n", stmt->data.let_stmt.is_const ? "true" : "false");
            if (stmt->data.let_stmt.type_annot) {
                indent(level + 1);
                printf("type: %s\n", stmt->data.let_stmt.type_annot->name);
            }
            indent(level + 1);
            printf("value:\n");
            print_expression(stmt->data.let_stmt.value, level + 2);
            break;
            
        case AST_ASSIGN_STMT:
            indent(level);
            printf("Assignment:\n");
            indent(level + 1);
            printf("name: %s\n", stmt->data.assign.name);
            indent(level + 1);
            printf("value:\n");
            print_expression(stmt->data.assign.value, level + 2);
            break;
            
        case AST_FUNC_DEF:
            indent(level);
            printf("FunctionDef:\n");
            indent(level + 1);
            printf("name: %s\n", stmt->data.func_def.name);
            
            if (stmt->data.func_def.params && stmt->data.func_def.params->size > 0) {
                indent(level + 1);
                printf("parameters:\n");
                for (size_t i = 0; i < stmt->data.func_def.params->size; i++) {
                    Parameter* p = stmt->data.func_def.params->items[i];
                    indent(level + 2);
                    printf("%s: %s\n", p->name, p->type->name);
                }
            }
            
            if (stmt->data.func_def.return_type) {
                indent(level + 1);
                printf("return_type: %s\n", stmt->data.func_def.return_type->name);
            }
            
            if (stmt->data.func_def.body && stmt->data.func_def.body->size > 0) {
                indent(level + 1);
                printf("body:\n");
                for (size_t i = 0; i < stmt->data.func_def.body->size; i++) {
                    print_statement(stmt->data.func_def.body->items[i], level + 2);
                }
            }
            break;
            
        case AST_RETURN_STMT:
            indent(level);
            printf("ReturnStatement:\n");
            if (stmt->data.return_stmt.value) {
                print_expression(stmt->data.return_stmt.value, level + 1);
            }
            break;
            
        case AST_IF_STMT:
            indent(level);
            printf("IfStatement:\n");
            indent(level + 1);
            printf("condition:\n");
            print_expression(stmt->data.if_stmt.condition, level + 2);
            
            if (stmt->data.if_stmt.then_branch && stmt->data.if_stmt.then_branch->size > 0) {
                indent(level + 1);
                printf("then:\n");
                for (size_t i = 0; i < stmt->data.if_stmt.then_branch->size; i++) {
                    print_statement(stmt->data.if_stmt.then_branch->items[i], level + 2);
                }
            }
            
            if (stmt->data.if_stmt.else_branch && stmt->data.if_stmt.else_branch->size > 0) {
                indent(level + 1);
                printf("else:\n");
                for (size_t i = 0; i < stmt->data.if_stmt.else_branch->size; i++) {
                    print_statement(stmt->data.if_stmt.else_branch->items[i], level + 2);
                }
            }
            break;
            
        case AST_WHILE_STMT:
            indent(level);
            printf("WhileStatement:\n");
            indent(level + 1);
            printf("condition:\n");
            print_expression(stmt->data.while_stmt.condition, level + 2);
            
            if (stmt->data.while_stmt.body && stmt->data.while_stmt.body->size > 0) {
                indent(level + 1);
                printf("body:\n");
                for (size_t i = 0; i < stmt->data.while_stmt.body->size; i++) {
                    print_statement(stmt->data.while_stmt.body->items[i], level + 2);
                }
            }
            break;
            
        case AST_FOR_STMT:
            indent(level);
            printf("ForStatement:\n");
            indent(level + 1);
            printf("variable: %s\n", stmt->data.for_stmt.var);
            indent(level + 1);
            printf("start:\n");
            print_expression(stmt->data.for_stmt.start, level + 2);
            indent(level + 1);
            printf("end:\n");
            print_expression(stmt->data.for_stmt.end, level + 2);
            
            if (stmt->data.for_stmt.body && stmt->data.for_stmt.body->size > 0) {
                indent(level + 1);
                printf("body:\n");
                for (size_t i = 0; i < stmt->data.for_stmt.body->size; i++) {
                    print_statement(stmt->data.for_stmt.body->items[i], level + 2);
                }
            }
            break;
            
        case AST_EXPR_STMT:
            indent(level);
            printf("ExpressionStatement:\n");
            print_expression(stmt->data.expr_stmt.expr, level + 1);
            break;
            
        default:
            indent(level);
            printf("Unknown statement type: %d\n", stmt->type);
    }
}

// Print entire AST
void ast_print(Program* prog) {
    if (!prog) {
        printf("(null program)\n");
        return;
    }
    
    printf("=== AST ===\n");
    printf("Program:\n");
    
    if (prog->statements && prog->statements->size > 0) {
        for (size_t i = 0; i < prog->statements->size; i++) {
            print_statement(prog->statements->items[i], 1);
        }
    } else {
        printf("  (empty)\n");
    }
}