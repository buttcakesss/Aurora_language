#ifndef PARSER_H
#define PARSER_H

#include "aurora_types.h"
#include "ast.h"

// Parser structure
typedef struct {
    TokenList* tokens;
    size_t current;
    ErrorInfo* error;
} Parser;

// Function prototypes
Parser* parser_create(TokenList* tokens);
void parser_destroy(Parser* parser);
Program* parser_parse(Parser* parser);

// Helper functions
Token* parser_peek(Parser* parser);
Token* parser_previous(Parser* parser);
Token* parser_advance(Parser* parser);
bool parser_match(Parser* parser, TokenType type);
bool parser_check(Parser* parser, TokenType type);
Token* parser_consume(Parser* parser, TokenType type, const char* message);
bool parser_is_at_end(Parser* parser);
void parser_skip_newlines(Parser* parser);

// Parsing functions
Statement* parser_declaration(Parser* parser);
Statement* parser_statement(Parser* parser);
Statement* parser_let_statement(Parser* parser, bool is_const);
Statement* parser_function_def(Parser* parser);
Statement* parser_if_statement(Parser* parser);
Statement* parser_while_statement(Parser* parser);
Statement* parser_for_statement(Parser* parser);
Statement* parser_return_statement(Parser* parser);
Statement* parser_expression_statement(Parser* parser);

Expression* parser_expression(Parser* parser);
Expression* parser_equality(Parser* parser);
Expression* parser_comparison(Parser* parser);
Expression* parser_term(Parser* parser);
Expression* parser_factor(Parser* parser);
Expression* parser_unary(Parser* parser);
Expression* parser_primary(Parser* parser);

#endif // PARSER_H