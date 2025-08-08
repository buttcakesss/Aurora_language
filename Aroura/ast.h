#ifndef AST_H
#define AST_H

#include "aurora_types.h"
#include <stdbool.h>

// Forward declarations
typedef struct ASTNode ASTNode;
typedef struct Program Program;
typedef struct Statement Statement;
typedef struct Expression Expression;

// AST Node types
typedef enum {
    // Statements
    AST_PROGRAM,
    AST_LET_STMT,
    AST_ASSIGN_STMT,
    AST_FUNC_DEF,
    AST_RETURN_STMT,
    AST_IF_STMT,
    AST_WHILE_STMT,
    AST_FOR_STMT,
    AST_EXPR_STMT,
    
    // Expressions
    AST_BINARY_OP,
    AST_UNARY_OP,
    AST_LITERAL,
    AST_VARIABLE,
    AST_CALL,
    
    // Literals
    AST_INT_LITERAL,
    AST_FLOAT_LITERAL,
    AST_STRING_LITERAL,
    AST_BOOL_LITERAL
} ASTNodeType;

// Type annotations
typedef struct {
    char* name;
} TypeAnnotation;

// Parameter for functions
typedef struct {
    char* name;
    TypeAnnotation* type;
} Parameter;

// Statement list
typedef struct {
    Statement** items;
    size_t size;
    size_t capacity;
} StatementList;

// Expression list (for function arguments)
typedef struct {
    Expression** items;
    size_t size;
    size_t capacity;
} ExpressionList;

// Parameter list
typedef struct {
    Parameter** items;
    size_t size;
    size_t capacity;
} ParameterList;

// Base expression struct
struct Expression {
    ASTNodeType type;
    union {
        // Binary operation
        struct {
            Expression* left;
            TokenType op;
            Expression* right;
        } binary;
        
        // Unary operation
        struct {
            TokenType op;
            Expression* operand;
        } unary;
        
        // Literals
        struct {
            union {
                int int_val;
                double float_val;
                char* string_val;
                bool bool_val;
            } value;
        } literal;
        
        // Variable reference
        struct {
            char* name;
        } variable;
        
        // Function call
        struct {
            char* name;
            ExpressionList* args;
        } call;
    } data;
};

// Base statement struct
struct Statement {
    ASTNodeType type;
    union {
        // Let statement
        struct {
            char* name;
            TypeAnnotation* type_annot;
            Expression* value;
            bool is_const;
        } let_stmt;
        
        // Assignment
        struct {
            char* name;
            Expression* value;
        } assign;
        
        // Function definition
        struct {
            char* name;
            ParameterList* params;
            TypeAnnotation* return_type;
            StatementList* body;
        } func_def;
        
        // Return statement
        struct {
            Expression* value;
        } return_stmt;
        
        // If statement
        struct {
            Expression* condition;
            StatementList* then_branch;
            StatementList* else_branch;
        } if_stmt;
        
        // While statement
        struct {
            Expression* condition;
            StatementList* body;
        } while_stmt;
        
        // For statement
        struct {
            char* var;
            Expression* start;
            Expression* end;
            StatementList* body;
        } for_stmt;
        
        // Expression statement
        struct {
            Expression* expr;
        } expr_stmt;
    } data;
};

// Program (root of AST)
struct Program {
    StatementList* statements;
};

// Function prototypes for AST manipulation
Program* program_create(void);
void program_destroy(Program* prog);

Statement* statement_create(ASTNodeType type);
void statement_destroy(Statement* stmt);

Expression* expression_create(ASTNodeType type);
void expression_destroy(Expression* expr);

StatementList* statement_list_create(void);
void statement_list_add(StatementList* list, Statement* stmt);
void statement_list_destroy(StatementList* list);

ExpressionList* expression_list_create(void);
void expression_list_add(ExpressionList* list, Expression* expr);
void expression_list_destroy(ExpressionList* list);

ParameterList* parameter_list_create(void);
void parameter_list_add(ParameterList* list, Parameter* param);
void parameter_list_destroy(ParameterList* list);

TypeAnnotation* type_annotation_create(const char* name);
void type_annotation_destroy(TypeAnnotation* type);

Parameter* parameter_create(const char* name, TypeAnnotation* type);
void parameter_destroy(Parameter* param);

// Debug functions
void ast_print(Program* prog);

#endif // AST_H