#include "parser.h"
#include "ast.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Create a new parser
Parser* parser_create(TokenList* tokens) {
    Parser* parser = (Parser*)malloc(sizeof(Parser));
    if (!parser) return NULL;
    
    parser->tokens = tokens;
    parser->current = 0;
    parser->error = (ErrorInfo*)calloc(1, sizeof(ErrorInfo));
    
    return parser;
}

// Destroy parser
void parser_destroy(Parser* parser) {
    if (parser) {
        free(parser->error);
        free(parser);
    }
}

// Get current token
Token* parser_peek(Parser* parser) {
    if (parser->current >= parser->tokens->size) {
        return &parser->tokens->tokens[parser->tokens->size - 1]; // Return EOF
    }
    return &parser->tokens->tokens[parser->current];
}

// Get previous token
Token* parser_previous(Parser* parser) {
    if (parser->current == 0) return &parser->tokens->tokens[0];
    return &parser->tokens->tokens[parser->current - 1];
}

// Advance to next token
Token* parser_advance(Parser* parser) {
    if (!parser_is_at_end(parser)) {
        parser->current++;
    }
    return parser_previous(parser);
}

// Check if current token matches type
bool parser_check(Parser* parser, TokenType type) {
    if (parser_is_at_end(parser)) return false;
    return parser_peek(parser)->type == type;
}

// Match and consume token if it matches
bool parser_match(Parser* parser, TokenType type) {
    if (parser_check(parser, type)) {
        parser_advance(parser);
        return true;
    }
    return false;
}

// Consume token or error
Token* parser_consume(Parser* parser, TokenType type, const char* message) {
    if (parser_check(parser, type)) {
        return parser_advance(parser);
    }
    
    Token* current = parser_peek(parser);
    parser->error->has_error = true;
    snprintf(parser->error->message, 256, 
             "[Line %d] %s. Found %s", 
             current->line, message, token_type_to_string(current->type));
    parser->error->line = current->line;
    parser->error->column = current->column;
    return NULL;
}

// Check if at end
bool parser_is_at_end(Parser* parser) {
    return parser_peek(parser)->type == TOK_EOF;
}

// Skip newline tokens
void parser_skip_newlines(Parser* parser) {
    while (parser_match(parser, TOK_NEWLINE)) {
        // Keep advancing
    }
}

// Helper: Skip optional semicolon
static void skip_optional_semicolon(Parser* parser) {
    parser_skip_newlines(parser);
    parser_match(parser, TOK_SEMICOLON);
    parser_skip_newlines(parser);
}

// Forward declarations for mutual recursion
static Statement* parse_declaration(Parser* parser);
static Statement* parse_statement(Parser* parser);
static Expression* parse_expression(Parser* parser);

// Parse let/const statement
Statement* parser_let_statement(Parser* parser, bool is_const) {
    parser_skip_newlines(parser);
    
    Token* name_token = parser_consume(parser, TOK_IDENT, "Expected variable name");
    if (!name_token) return NULL;
    
    Statement* stmt = statement_create(AST_LET_STMT);
    stmt->data.let_stmt.name = strdup(name_token->value);
    stmt->data.let_stmt.is_const = is_const;
    stmt->data.let_stmt.type_annot = NULL;
    
    parser_skip_newlines(parser);
    
    // Optional type annotation
    if (parser_match(parser, TOK_COLON)) {
        parser_skip_newlines(parser);
        
        TokenType type_tokens[] = {
            TOK_TYPE_INT, TOK_TYPE_FLOAT, TOK_TYPE_BOOL, 
            TOK_TYPE_STRING, TOK_TYPE_VOID
        };
        
        bool found_type = false;
        for (int i = 0; i < 5; i++) {
            if (parser_match(parser, type_tokens[i])) {
                Token* type_token = parser_previous(parser);
                stmt->data.let_stmt.type_annot = type_annotation_create(type_token->value);
                found_type = true;
                break;
            }
        }
        
        if (!found_type && parser_check(parser, TOK_IDENT)) {
            Token* custom_type = parser_advance(parser);
            stmt->data.let_stmt.type_annot = type_annotation_create(custom_type->value);
        }
    }
    
    parser_skip_newlines(parser);
    
    if (!parser_consume(parser, TOK_ASSIGN, "Expected '=' after variable")) {
        statement_destroy(stmt);
        return NULL;
    }
    
    stmt->data.let_stmt.value = parse_expression(parser);
    if (!stmt->data.let_stmt.value) {
        statement_destroy(stmt);
        return NULL;
    }
    
    return stmt;
}

// Parse function definition
Statement* parser_function_def(Parser* parser) {
    Token* name = parser_consume(parser, TOK_IDENT, "Expected function name");
    if (!name) return NULL;
    
    Statement* func = statement_create(AST_FUNC_DEF);
    func->data.func_def.name = strdup(name->value);
    func->data.func_def.params = parameter_list_create();
    
    if (!parser_consume(parser, TOK_LPAREN, "Expected '(' after function name")) {
        statement_destroy(func);
        return NULL;
    }
    
    // Parse parameters
    while (!parser_check(parser, TOK_RPAREN)) {
        parser_skip_newlines(parser);
        
        Token* param_name = parser_consume(parser, TOK_IDENT, "Expected parameter name");
        if (!param_name) {
            statement_destroy(func);
            return NULL;
        }
        
        if (!parser_consume(parser, TOK_COLON, "Expected ':' after parameter name")) {
            statement_destroy(func);
            return NULL;
        }
        
        // Parse parameter type
        TypeAnnotation* param_type = NULL;
        TokenType type_tokens[] = {
            TOK_TYPE_INT, TOK_TYPE_FLOAT, TOK_TYPE_BOOL, 
            TOK_TYPE_STRING, TOK_TYPE_VOID
        };
        
        for (int i = 0; i < 5; i++) {
            if (parser_match(parser, type_tokens[i])) {
                param_type = type_annotation_create(parser_previous(parser)->value);
                break;
            }
        }
        
        if (!param_type && parser_check(parser, TOK_IDENT)) {
            Token* custom = parser_advance(parser);
            param_type = type_annotation_create(custom->value);
        }
        
        if (!param_type) {
            parser->error->has_error = true;
            snprintf(parser->error->message, 256, "Expected parameter type");
            statement_destroy(func);
            return NULL;
        }
        
        Parameter* param = parameter_create(param_name->value, param_type);
        parameter_list_add(func->data.func_def.params, param);
        
        if (parser_check(parser, TOK_COMMA)) {
            parser_advance(parser);
        }
    }
    
    if (!parser_consume(parser, TOK_RPAREN, "Expected ')' after parameters")) {
        statement_destroy(func);
        return NULL;
    }
    
    if (!parser_consume(parser, TOK_ARROW, "Expected '->' for return type")) {
        statement_destroy(func);
        return NULL;
    }
    
    // Parse return type
    TokenType type_tokens[] = {
        TOK_TYPE_INT, TOK_TYPE_FLOAT, TOK_TYPE_BOOL, 
        TOK_TYPE_STRING, TOK_TYPE_VOID
    };
    
    for (int i = 0; i < 5; i++) {
        if (parser_match(parser, type_tokens[i])) {
            func->data.func_def.return_type = type_annotation_create(parser_previous(parser)->value);
            break;
        }
    }
    
    if (!func->data.func_def.return_type && parser_check(parser, TOK_IDENT)) {
        Token* custom = parser_advance(parser);
        func->data.func_def.return_type = type_annotation_create(custom->value);
    }
    
    if (!func->data.func_def.return_type) {
        parser->error->has_error = true;
        snprintf(parser->error->message, 256, "Expected return type");
        statement_destroy(func);
        return NULL;
    }
    
    if (!parser_consume(parser, TOK_LBRACE, "Expected '{' before function body")) {
        statement_destroy(func);
        return NULL;
    }
    
    // Parse function body
    func->data.func_def.body = statement_list_create();
    parser_skip_newlines(parser);
    
    while (!parser_check(parser, TOK_RBRACE)) {
        Statement* stmt = parse_declaration(parser);
        if (!stmt) {
            statement_destroy(func);
            return NULL;
        }
        statement_list_add(func->data.func_def.body, stmt);
        parser_skip_newlines(parser);
    }
    
    if (!parser_consume(parser, TOK_RBRACE, "Expected '}' after function body")) {
        statement_destroy(func);
        return NULL;
    }
    
    return func;
}

// Parse if statement
Statement* parser_if_statement(Parser* parser) {
    Statement* stmt = statement_create(AST_IF_STMT);
    
    stmt->data.if_stmt.condition = parse_expression(parser);
    if (!stmt->data.if_stmt.condition) {
        statement_destroy(stmt);
        return NULL;
    }
    
    if (!parser_consume(parser, TOK_LBRACE, "Expected '{' after if condition")) {
        statement_destroy(stmt);
        return NULL;
    }
    
    stmt->data.if_stmt.then_branch = statement_list_create();
    parser_skip_newlines(parser);
    
    while (!parser_check(parser, TOK_RBRACE)) {
        Statement* s = parse_declaration(parser);
        if (!s) {
            statement_destroy(stmt);
            return NULL;
        }
        statement_list_add(stmt->data.if_stmt.then_branch, s);
        parser_skip_newlines(parser);
    }
    
    if (!parser_consume(parser, TOK_RBRACE, "Expected '}' after if body")) {
        statement_destroy(stmt);
        return NULL;
    }
    
    // Optional else branch
    stmt->data.if_stmt.else_branch = NULL;
    if (parser_match(parser, TOK_ELSE)) {
        if (!parser_consume(parser, TOK_LBRACE, "Expected '{' after else")) {
            statement_destroy(stmt);
            return NULL;
        }
        
        stmt->data.if_stmt.else_branch = statement_list_create();
        parser_skip_newlines(parser);
        
        while (!parser_check(parser, TOK_RBRACE)) {
            Statement* s = parse_declaration(parser);
            if (!s) {
                statement_destroy(stmt);
                return NULL;
            }
            statement_list_add(stmt->data.if_stmt.else_branch, s);
            parser_skip_newlines(parser);
        }
        
        if (!parser_consume(parser, TOK_RBRACE, "Expected '}' after else body")) {
            statement_destroy(stmt);
            return NULL;
        }
    }
    
    return stmt;
}

// Parse while statement
Statement* parser_while_statement(Parser* parser) {
    Statement* stmt = statement_create(AST_WHILE_STMT);
    
    stmt->data.while_stmt.condition = parse_expression(parser);
    if (!stmt->data.while_stmt.condition) {
        statement_destroy(stmt);
        return NULL;
    }
    
    if (!parser_consume(parser, TOK_LBRACE, "Expected '{' after while condition")) {
        statement_destroy(stmt);
        return NULL;
    }
    
    stmt->data.while_stmt.body = statement_list_create();
    parser_skip_newlines(parser);
    
    while (!parser_check(parser, TOK_RBRACE)) {
        Statement* s = parse_declaration(parser);
        if (!s) {
            statement_destroy(stmt);
            return NULL;
        }
        statement_list_add(stmt->data.while_stmt.body, s);
        parser_skip_newlines(parser);
    }
    
    if (!parser_consume(parser, TOK_RBRACE, "Expected '}' after while body")) {
        statement_destroy(stmt);
        return NULL;
    }
    
    return stmt;
}

// Parse for statement
Statement* parser_for_statement(Parser* parser) {
    Statement* stmt = statement_create(AST_FOR_STMT);
    
    Token* var = parser_consume(parser, TOK_IDENT, "Expected iterator variable");
    if (!var) {
        statement_destroy(stmt);
        return NULL;
    }
    stmt->data.for_stmt.var = strdup(var->value);
    
    if (!parser_consume(parser, TOK_IN, "Expected 'in'")) {
        statement_destroy(stmt);
        return NULL;
    }
    
    if (!parser_consume(parser, TOK_RANGE, "Expected 'range'")) {
        statement_destroy(stmt);
        return NULL;
    }
    
    if (!parser_consume(parser, TOK_LPAREN, "Expected '(' in range")) {
        statement_destroy(stmt);
        return NULL;
    }
    
    stmt->data.for_stmt.start = parse_expression(parser);
    if (!stmt->data.for_stmt.start) {
        statement_destroy(stmt);
        return NULL;
    }
    
    if (!parser_consume(parser, TOK_COMMA, "Expected ','")) {
        statement_destroy(stmt);
        return NULL;
    }
    
    stmt->data.for_stmt.end = parse_expression(parser);
    if (!stmt->data.for_stmt.end) {
        statement_destroy(stmt);
        return NULL;
    }
    
    if (!parser_consume(parser, TOK_RPAREN, "Expected ')'")) {
        statement_destroy(stmt);
        return NULL;
    }
    
    if (!parser_consume(parser, TOK_LBRACE, "Expected '{'")) {
        statement_destroy(stmt);
        return NULL;
    }
    
    stmt->data.for_stmt.body = statement_list_create();
    parser_skip_newlines(parser);
    
    while (!parser_check(parser, TOK_RBRACE)) {
        Statement* s = parse_declaration(parser);
        if (!s) {
            statement_destroy(stmt);
            return NULL;
        }
        statement_list_add(stmt->data.for_stmt.body, s);
        parser_skip_newlines(parser);
    }
    
    if (!parser_consume(parser, TOK_RBRACE, "Expected '}'")) {
        statement_destroy(stmt);
        return NULL;
    }
    
    return stmt;
}

// Parse return statement
Statement* parser_return_statement(Parser* parser) {
    Statement* stmt = statement_create(AST_RETURN_STMT);
    stmt->data.return_stmt.value = parse_expression(parser);
    return stmt;
}

// Parse expression statement
Statement* parser_expression_statement(Parser* parser) {
    Statement* stmt = statement_create(AST_EXPR_STMT);
    stmt->data.expr_stmt.expr = parse_expression(parser);
    if (!stmt->data.expr_stmt.expr) {
        statement_destroy(stmt);
        return NULL;
    }
    return stmt;
}

// Parse statement
static Statement* parse_statement(Parser* parser) {
    parser_skip_newlines(parser);
    
    // Check for assignment: IDENT '=' expr
    if (parser_check(parser, TOK_IDENT) && 
        parser->current + 1 < parser->tokens->size &&
        parser->tokens->tokens[parser->current + 1].type == TOK_ASSIGN) {
        
        Token* name = parser_advance(parser);
        parser_consume(parser, TOK_ASSIGN, "Expected '=' in assignment");
        
        Statement* stmt = statement_create(AST_ASSIGN_STMT);
        stmt->data.assign.name = strdup(name->value);
        stmt->data.assign.value = parse_expression(parser);
        
        if (!stmt->data.assign.value) {
            statement_destroy(stmt);
            return NULL;
        }
        return stmt;
    }
    
    if (parser_match(parser, TOK_RETURN)) {
        return parser_return_statement(parser);
    }
    
    if (parser_match(parser, TOK_IF)) {
        return parser_if_statement(parser);
    }
    
    if (parser_match(parser, TOK_WHILE)) {
        return parser_while_statement(parser);
    }
    
    if (parser_match(parser, TOK_FOR)) {
        return parser_for_statement(parser);
    }
    
    // Expression statement
    return parser_expression_statement(parser);
}

// Parse declaration
static Statement* parse_declaration(Parser* parser) {
    if (parser_match(parser, TOK_LET)) {
        Statement* stmt = parser_let_statement(parser, false);
        skip_optional_semicolon(parser);
        return stmt;
    }
    
    if (parser_match(parser, TOK_CONST)) {
        Statement* stmt = parser_let_statement(parser, true);
        skip_optional_semicolon(parser);
        return stmt;
    }
    
    if (parser_match(parser, TOK_FUNC)) {
        return parser_function_def(parser);
    }
    
    Statement* stmt = parse_statement(parser);
    skip_optional_semicolon(parser);
    return stmt;
}

// Expression parsing (precedence climbing)

// Parse primary expression
Expression* parser_primary(Parser* parser) {
    // Handle parentheses for grouping
    if (parser_match(parser, TOK_LPAREN)) {
        Expression* expr = parse_expression(parser);
        parser_consume(parser, TOK_RPAREN, "Expected ')' after expression");
        return expr;
    }
    
    // Number literal
    if (parser_check(parser, TOK_NUMBER)) {
        Token* num = parser_advance(parser);
        Expression* expr = expression_create(AST_LITERAL);
        
        // Check if it's a float or int
        if (strchr(num->value, '.')) {
            expr->data.literal.value.float_val = atof(num->value);
            expr->type = AST_FLOAT_LITERAL;
        } else {
            expr->data.literal.value.int_val = atoi(num->value);
            expr->type = AST_INT_LITERAL;
        }
        return expr;
    }
    
    // String literal
    if (parser_check(parser, TOK_STRING_LITERAL)) {
        Token* str = parser_advance(parser);
        Expression* expr = expression_create(AST_STRING_LITERAL);
        expr->data.literal.value.string_val = strdup(str->value);
        return expr;
    }
    
    // Boolean literal
    if (parser_check(parser, TOK_BOOL_LITERAL)) {
        Token* bool_tok = parser_advance(parser);
        Expression* expr = expression_create(AST_BOOL_LITERAL);
        expr->data.literal.value.bool_val = strcmp(bool_tok->value, "true") == 0;
        return expr;
    }
    
    // Identifier (variable or function call)
    if (parser_check(parser, TOK_IDENT)) {
        Token* ident = parser_advance(parser);
        
        // Check for function call
        if (parser_check(parser, TOK_LPAREN)) {
            parser_advance(parser); // consume '('
            
            Expression* expr = expression_create(AST_CALL);
            expr->data.call.name = strdup(ident->value);
            expr->data.call.args = expression_list_create();
            
            while (!parser_check(parser, TOK_RPAREN)) {
                Expression* arg = parse_expression(parser);
                if (!arg) {
                    expression_destroy(expr);
                    return NULL;
                }
                expression_list_add(expr->data.call.args, arg);
                
                if (parser_check(parser, TOK_COMMA)) {
                    parser_advance(parser);
                }
            }
            
            parser_consume(parser, TOK_RPAREN, "Expected ')'");
            return expr;
        }
        
        // Variable reference
        Expression* expr = expression_create(AST_VARIABLE);
        expr->data.variable.name = strdup(ident->value);
        return expr;
    }
    
    parser->error->has_error = true;
    Token* tok = parser_peek(parser);
    snprintf(parser->error->message, 256, 
             "[Line %d] Unexpected token: %s", 
             tok->line, token_type_to_string(tok->type));
    return NULL;
}

// Parse unary expression
Expression* parser_unary(Parser* parser) {
    if (parser_match(parser, TOK_MINUS)) {
        Expression* expr = expression_create(AST_UNARY_OP);
        expr->data.unary.op = TOK_MINUS;
        expr->data.unary.operand = parser_unary(parser);
        return expr;
    }
    
    return parser_primary(parser);
}

// Parse factor (*, /, %)
Expression* parser_factor(Parser* parser) {
    Expression* expr = parser_unary(parser);
    
    while (parser_match(parser, TOK_STAR) || 
           parser_match(parser, TOK_SLASH) || 
           parser_match(parser, TOK_MOD)) {
        TokenType op = parser_previous(parser)->type;
        Expression* right = parser_unary(parser);
        
        Expression* binary = expression_create(AST_BINARY_OP);
        binary->data.binary.left = expr;
        binary->data.binary.op = op;
        binary->data.binary.right = right;
        expr = binary;
    }
    
    return expr;
}

// Parse term (+, -)
Expression* parser_term(Parser* parser) {
    Expression* expr = parser_factor(parser);
    
    while (parser_match(parser, TOK_PLUS) || 
           parser_match(parser, TOK_MINUS)) {
        TokenType op = parser_previous(parser)->type;
        Expression* right = parser_factor(parser);
        
        Expression* binary = expression_create(AST_BINARY_OP);
        binary->data.binary.left = expr;
        binary->data.binary.op = op;
        binary->data.binary.right = right;
        expr = binary;
    }
    
    return expr;
}

// Parse comparison (<, >, <=, >=)
Expression* parser_comparison(Parser* parser) {
    Expression* expr = parser_term(parser);
    
    while (parser_match(parser, TOK_LT) || 
           parser_match(parser, TOK_GT) ||
           parser_match(parser, TOK_LTE) || 
           parser_match(parser, TOK_GTE)) {
        TokenType op = parser_previous(parser)->type;
        Expression* right = parser_term(parser);
        
        Expression* binary = expression_create(AST_BINARY_OP);
        binary->data.binary.left = expr;
        binary->data.binary.op = op;
        binary->data.binary.right = right;
        expr = binary;
    }
    
    return expr;
}

// Parse equality (==, !=)
Expression* parser_equality(Parser* parser) {
    Expression* expr = parser_comparison(parser);
    
    while (parser_match(parser, TOK_EQEQ) || 
           parser_match(parser, TOK_NEQ)) {
        TokenType op = parser_previous(parser)->type;
        Expression* right = parser_comparison(parser);
        
        Expression* binary = expression_create(AST_BINARY_OP);
        binary->data.binary.left = expr;
        binary->data.binary.op = op;
        binary->data.binary.right = right;
        expr = binary;
    }
    
    return expr;
}

// Parse expression
static Expression* parse_expression(Parser* parser) {
    return parser_equality(parser);
}

// Main parse function
Program* parser_parse(Parser* parser) {
    Program* program = program_create();
    
    while (!parser_is_at_end(parser)) {
        parser_skip_newlines(parser);
        if (parser_is_at_end(parser)) break;
        
        Statement* stmt = parse_declaration(parser);
        if (!stmt) {
            if (parser->error->has_error) {
                program_destroy(program);
                return NULL;
            }
            break;
        }
        
        statement_list_add(program->statements, stmt);
        parser_skip_newlines(parser);
    }
    
    return program;
}

// Public parsing wrappers
Statement* parser_declaration(Parser* parser) {
    return parse_declaration(parser);
}

Statement* parser_statement(Parser* parser) {
    return parse_statement(parser);
}

Expression* parser_expression(Parser* parser) {
    return parse_expression(parser);
}