#include "lexer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

// Keyword table
typedef struct {
    const char* keyword;
    TokenType type;
} KeywordEntry;

static const KeywordEntry keywords[] = {
    {"let", TOK_LET},
    {"const", TOK_CONST},
    {"func", TOK_FUNC},
    {"return", TOK_RETURN},
    {"if", TOK_IF},
    {"else", TOK_ELSE},
    {"while", TOK_WHILE},
    {"for", TOK_FOR},
    {"in", TOK_IN},
    {"range", TOK_RANGE},
    {"int", TOK_TYPE_INT},
    {"float", TOK_TYPE_FLOAT},
    {"bool", TOK_TYPE_BOOL},
    {"string", TOK_TYPE_STRING},
    {"void", TOK_TYPE_VOID},
    {"true", TOK_BOOL_LITERAL},
    {"false", TOK_BOOL_LITERAL},
    {NULL, 0}
};

// Create a new lexer
Lexer* lexer_create(const char* source) {
    Lexer* lexer = (Lexer*)malloc(sizeof(Lexer));
    if (!lexer) return NULL;
    
    lexer->source = source;
    lexer->pos = 0;
    lexer->length = strlen(source);
    lexer->line = 1;
    lexer->column = 1;
    lexer->error = (ErrorInfo*)calloc(1, sizeof(ErrorInfo));
    
    return lexer;
}

// Destroy lexer
void lexer_destroy(Lexer* lexer) {
    if (lexer) {
        free(lexer->error);
        free(lexer);
    }
}

// Helper: Current character
static char current_char(Lexer* lexer) {
    if (lexer->pos >= lexer->length) return '\0';
    return lexer->source[lexer->pos];
}

// Helper: Peek ahead
static char peek_char(Lexer* lexer, int offset) {
    size_t pos = lexer->pos + offset;
    if (pos >= lexer->length) return '\0';
    return lexer->source[pos];
}

// Helper: Advance position
static char advance(Lexer* lexer) {
    char ch = current_char(lexer);
    if (ch != '\0') {
        lexer->pos++;
        if (ch == '\n') {
            lexer->line++;
            lexer->column = 1;
        } else {
            lexer->column++;
        }
    }
    return ch;
}

// Helper: Skip whitespace
static void skip_whitespace(Lexer* lexer) {
    while (1) {
        char ch = current_char(lexer);
        if (ch == ' ' || ch == '\t' || ch == '\r') {
            advance(lexer);
        } else if (ch == '/' && peek_char(lexer, 1) == '/') {
            // Skip line comment
            while (current_char(lexer) != '\n' && current_char(lexer) != '\0') {
                advance(lexer);
            }
        } else {
            break;
        }
    }
}

// Helper: Create token
static Token create_token(TokenType type, const char* value, int line, int column) {
    Token token;
    token.type = type;
    token.value = value ? strdup(value) : NULL;
    token.line = line;
    token.column = column;
    return token;
}

// Helper: Read string literal
static Token read_string(Lexer* lexer) {
    int start_line = lexer->line;
    int start_col = lexer->column;
    advance(lexer); // Skip opening quote
    
    char buffer[1024];
    int i = 0;
    
    while (current_char(lexer) != '"' && current_char(lexer) != '\0') {
        if (i >= 1023) {
            lexer->error->has_error = true;
            snprintf(lexer->error->message, 256, "String too long at line %d", lexer->line);
            return create_token(TOK_ERROR, NULL, start_line, start_col);
        }
        buffer[i++] = advance(lexer);
    }
    
    if (current_char(lexer) != '"') {
        lexer->error->has_error = true;
        snprintf(lexer->error->message, 256, "Unterminated string at line %d", start_line);
        return create_token(TOK_ERROR, NULL, start_line, start_col);
    }
    
    advance(lexer); // Skip closing quote
    buffer[i] = '\0';
    return create_token(TOK_STRING_LITERAL, buffer, start_line, start_col);
}

// Helper: Read number
static Token read_number(Lexer* lexer) {
    int start_line = lexer->line;
    int start_col = lexer->column;
    char buffer[64];
    int i = 0;
    bool has_dot = false;
    
    while (isdigit(current_char(lexer)) || 
           (current_char(lexer) == '.' && !has_dot && isdigit(peek_char(lexer, 1)))) {
        if (current_char(lexer) == '.') has_dot = true;
        if (i >= 63) break;
        buffer[i++] = advance(lexer);
    }
    
    buffer[i] = '\0';
    return create_token(TOK_NUMBER, buffer, start_line, start_col);
}

// Helper: Read identifier
static Token read_identifier(Lexer* lexer) {
    int start_line = lexer->line;
    int start_col = lexer->column;
    char buffer[256];
    int i = 0;
    
    while (isalnum(current_char(lexer)) || current_char(lexer) == '_') {
        if (i >= 255) break;
        buffer[i++] = advance(lexer);
    }
    
    buffer[i] = '\0';
    
    // Check if it's a keyword
    for (const KeywordEntry* kw = keywords; kw->keyword != NULL; kw++) {
        if (strcmp(buffer, kw->keyword) == 0) {
            return create_token(kw->type, buffer, start_line, start_col);
        }
    }
    
    return create_token(TOK_IDENT, buffer, start_line, start_col);
}

// Create token list
static TokenList* token_list_create() {
    TokenList* list = (TokenList*)malloc(sizeof(TokenList));
    if (!list) return NULL;
    
    list->capacity = 16;
    list->size = 0;
    list->tokens = (Token*)malloc(sizeof(Token) * list->capacity);
    
    return list;
}

// Add token to list
void token_list_add(TokenList* list, Token token) {
    if (list->size >= list->capacity) {
        list->capacity *= 2;
        list->tokens = (Token*)realloc(list->tokens, sizeof(Token) * list->capacity);
    }
    list->tokens[list->size++] = token;
}

// Destroy token list
void token_list_destroy(TokenList* tokens) {
    if (tokens) {
        for (size_t i = 0; i < tokens->size; i++) {
            free(tokens->tokens[i].value);
        }
        free(tokens->tokens);
        free(tokens);
    }
}

// Main tokenization function
TokenList* lexer_tokenize(Lexer* lexer) {
    TokenList* tokens = token_list_create();
    if (!tokens) return NULL;
    
    while (current_char(lexer) != '\0') {
        skip_whitespace(lexer);
        
        if (current_char(lexer) == '\0') break;
        
        int start_line = lexer->line;
        int start_col = lexer->column;
        char ch = current_char(lexer);
        
        // Newline
        if (ch == '\n') {
            token_list_add(tokens, create_token(TOK_NEWLINE, "\n", start_line, start_col));
            advance(lexer);
            continue;
        }
        
        // String literal
        if (ch == '"') {
            token_list_add(tokens, read_string(lexer));
            continue;
        }
        
        // Number
        if (isdigit(ch)) {
            token_list_add(tokens, read_number(lexer));
            continue;
        }
        
        // Identifier or keyword
        if (isalpha(ch) || ch == '_') {
            token_list_add(tokens, read_identifier(lexer));
            continue;
        }
        
        // Two-character operators
        char next = peek_char(lexer, 1);
        if (ch == '-' && next == '>') {
            advance(lexer); advance(lexer);
            token_list_add(tokens, create_token(TOK_ARROW, "->", start_line, start_col));
            continue;
        }
        if (ch == '=' && next == '=') {
            advance(lexer); advance(lexer);
            token_list_add(tokens, create_token(TOK_EQEQ, "==", start_line, start_col));
            continue;
        }
        if (ch == '!' && next == '=') {
            advance(lexer); advance(lexer);
            token_list_add(tokens, create_token(TOK_NEQ, "!=", start_line, start_col));
            continue;
        }
        if (ch == '<' && next == '=') {
            advance(lexer); advance(lexer);
            token_list_add(tokens, create_token(TOK_LTE, "<=", start_line, start_col));
            continue;
        }
        if (ch == '>' && next == '=') {
            advance(lexer); advance(lexer);
            token_list_add(tokens, create_token(TOK_GTE, ">=", start_line, start_col));
            continue;
        }
        
        // Single-character tokens
        TokenType type = TOK_ERROR;
        char value[2] = {ch, '\0'};
        
        switch (ch) {
            case '+': type = TOK_PLUS; break;
            case '-': type = TOK_MINUS; break;
            case '*': type = TOK_STAR; break;
            case '/': type = TOK_SLASH; break;
            case '%': type = TOK_MOD; break;
            case '=': type = TOK_ASSIGN; break;
            case '<': type = TOK_LT; break;
            case '>': type = TOK_GT; break;
            case '(': type = TOK_LPAREN; break;
            case ')': type = TOK_RPAREN; break;
            case '{': type = TOK_LBRACE; break;
            case '}': type = TOK_RBRACE; break;
            case '[': type = TOK_LBRACKET; break;
            case ']': type = TOK_RBRACKET; break;
            case ':': type = TOK_COLON; break;
            case ',': type = TOK_COMMA; break;
            case ';': type = TOK_SEMICOLON; break;
            default:
                lexer->error->has_error = true;
                snprintf(lexer->error->message, 256, 
                        "Unexpected character '%c' at line %d, column %d", 
                        ch, start_line, start_col);
                token_list_add(tokens, create_token(TOK_ERROR, value, start_line, start_col));
                advance(lexer);
                continue;
        }
        
        advance(lexer);
        token_list_add(tokens, create_token(type, value, start_line, start_col));
    }
    
    // Add EOF token
    token_list_add(tokens, create_token(TOK_EOF, "", lexer->line, lexer->column));
    
    return tokens;
}

// Debug: Convert token type to string
const char* token_type_to_string(TokenType type) {
    switch (type) {
        case TOK_LET: return "LET";
        case TOK_CONST: return "CONST";
        case TOK_FUNC: return "FUNC";
        case TOK_RETURN: return "RETURN";
        case TOK_IF: return "IF";
        case TOK_ELSE: return "ELSE";
        case TOK_WHILE: return "WHILE";
        case TOK_FOR: return "FOR";
        case TOK_IN: return "IN";
        case TOK_RANGE: return "RANGE";
        case TOK_TYPE_INT: return "INT";
        case TOK_TYPE_FLOAT: return "FLOAT";
        case TOK_TYPE_BOOL: return "BOOL";
        case TOK_TYPE_STRING: return "STRING";
        case TOK_TYPE_VOID: return "VOID";
        case TOK_PLUS: return "PLUS";
        case TOK_MINUS: return "MINUS";
        case TOK_STAR: return "STAR";
        case TOK_SLASH: return "SLASH";
        case TOK_MOD: return "MOD";
        case TOK_EQEQ: return "EQEQ";
        case TOK_NEQ: return "NEQ";
        case TOK_LT: return "LT";
        case TOK_GT: return "GT";
        case TOK_LTE: return "LTE";
        case TOK_GTE: return "GTE";
        case TOK_LPAREN: return "LPAREN";
        case TOK_RPAREN: return "RPAREN";
        case TOK_LBRACE: return "LBRACE";
        case TOK_RBRACE: return "RBRACE";
        case TOK_LBRACKET: return "LBRACKET";
        case TOK_RBRACKET: return "RBRACKET";
        case TOK_COLON: return "COLON";
        case TOK_COMMA: return "COMMA";
        case TOK_ARROW: return "ARROW";
        case TOK_ASSIGN: return "ASSIGN";
        case TOK_SEMICOLON: return "SEMICOLON";
        case TOK_IDENT: return "IDENT";
        case TOK_NUMBER: return "NUMBER";
        case TOK_STRING_LITERAL: return "STRING_LITERAL";
        case TOK_BOOL_LITERAL: return "BOOL_LITERAL";
        case TOK_NEWLINE: return "NEWLINE";
        case TOK_EOF: return "EOF";
        case TOK_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

// Debug: Print token
void token_print(Token* token) {
    printf("Token(%s, \"%s\", %d:%d)\n", 
           token_type_to_string(token->type),
           token->value ? token->value : "",
           token->line, 
           token->column);
}