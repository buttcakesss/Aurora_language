#ifndef LEXER_H
#define LEXER_H

#include "aurora_types.h"
#include <stddef.h>

// Lexer structure
typedef struct {
    const char* source;
    size_t pos;
    size_t length;
    int line;
    int column;
    ErrorInfo* error;
} Lexer;

// Function prototypes
Lexer* lexer_create(const char* source);
void lexer_destroy(Lexer* lexer);
TokenList* lexer_tokenize(Lexer* lexer);
void token_list_destroy(TokenList* tokens);
void token_list_add(TokenList* list, Token token);
const char* token_type_to_string(TokenType type);
void token_print(Token* token);

#endif // LEXER_H