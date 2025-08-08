#ifndef AURORA_TYPES_H
#define AURORA_TYPES_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>  // for size_t

// Token types enum
typedef enum {
    // Keywords
    TOK_LET,
    TOK_CONST,
    TOK_FUNC,
    TOK_RETURN,
    TOK_IF,
    TOK_ELSE,
    TOK_WHILE,
    TOK_FOR,
    TOK_IN,
    TOK_RANGE,
    
    // Types
    TOK_TYPE_INT,
    TOK_TYPE_FLOAT,
    TOK_TYPE_BOOL,
    TOK_TYPE_STRING,
    TOK_TYPE_VOID,
    
    // Operators
    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR,
    TOK_SLASH,
    TOK_MOD,
    TOK_EQEQ,
    TOK_NEQ,
    TOK_LT,
    TOK_GT,
    TOK_LTE,
    TOK_GTE,
    
    // Symbols
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_LBRACKET,
    TOK_RBRACKET,
    TOK_COLON,
    TOK_COMMA,
    TOK_ARROW,
    TOK_ASSIGN,
    TOK_SEMICOLON,
    
    // Literals/identifiers
    TOK_IDENT,
    TOK_NUMBER,
    TOK_STRING_LITERAL,
    TOK_BOOL_LITERAL,
    
    // Special
    TOK_NEWLINE,
    TOK_EOF,
    TOK_ERROR
} TokenType;

// Token structure
typedef struct {
    TokenType type;
    char* value;
    int line;
    int column;
} Token;

// Dynamic array for tokens
typedef struct {
    Token* tokens;
    size_t size;
    size_t capacity;
} TokenList;

// Error handling
typedef struct {
    bool has_error;
    char message[256];
    int line;
    int column;
} ErrorInfo;

#endif // AURORA_TYPES_H