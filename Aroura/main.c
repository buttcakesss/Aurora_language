#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "codegen_wasm.h"

#define VERSION "1.0.0"

// Function prototypes
static void print_usage(void);
static void print_version(void);
static char* read_file(const char* filename);
static bool file_exists(const char* filename);
static int compile_file(const char* input_file, const char* output_file, bool verbose);
static int run_file(const char* input_file, bool verbose);

// Print usage information
static void print_usage(void) {
    printf("Aurora Language Compiler v%s\n", VERSION);
    printf("\n");
    printf("Usage:\n");
    printf("  aurora build <file.aur> [output.wasm]  # Compile to WASM\n");
    printf("  aurora run <file.aur>                  # Compile and run\n");
    printf("  aurora lex <file.aur>                  # Show tokens (debug)\n");
    printf("  aurora parse <file.aur>                # Show AST (debug)\n");
    printf("  aurora wat <file.aur>                  # Show WAT output (debug)\n");
    printf("  aurora version                         # Show version info\n");
    printf("\n");
    printf("Options:\n");
    printf("  -v, --verbose                          # Verbose output\n");
    printf("\n");
    printf("Examples:\n");
    printf("  aurora build hello.aur                 # Creates hello.wasm\n");
    printf("  aurora build hello.aur app.wasm        # Creates app.wasm\n");
    printf("  aurora run hello.aur                   # Compile and execute\n");
}

// Print version information
static void print_version(void) {
    printf("Aurora Language Compiler v%s\n", VERSION);
    printf("Target: WebAssembly (WASM)\n");
    printf("Written in C\n");
}

// Check if file exists
static bool file_exists(const char* filename) {
    struct stat st;
    return stat(filename, &st) == 0;
}

// Read entire file into memory
static char* read_file(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        return NULL;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Allocate buffer
    char* buffer = (char*)malloc(size + 1);
    if (!buffer) {
        fclose(file);
        return NULL;
    }
    
    // Read file
    size_t read_size = fread(buffer, 1, size, file);
    buffer[read_size] = '\0';
    
    fclose(file);
    return buffer;
}

// Debug: Show tokens
static int debug_lex(const char* input_file) {
    if (!file_exists(input_file)) {
        fprintf(stderr, "Error: File '%s' not found.\n", input_file);
        return 1;
    }
    
    char* source = read_file(input_file);
    if (!source) {
        fprintf(stderr, "Error: Failed to read file '%s'.\n", input_file);
        return 1;
    }
    
    Lexer* lexer = lexer_create(source);
    if (!lexer) {
        fprintf(stderr, "Error: Failed to create lexer.\n");
        free(source);
        return 1;
    }
    
    TokenList* tokens = lexer_tokenize(lexer);
    if (!tokens) {
        fprintf(stderr, "Error: Tokenization failed.\n");
        lexer_destroy(lexer);
        free(source);
        return 1;
    }
    
    // Print tokens
    printf("=== TOKENS ===\n");
    for (size_t i = 0; i < tokens->size; i++) {
        token_print(&tokens->tokens[i]);
    }
    printf("Total tokens: %zu\n", tokens->size);
    
    // Check for errors
    if (lexer->error->has_error) {
        fprintf(stderr, "Lexer error: %s\n", lexer->error->message);
    }
    
    token_list_destroy(tokens);
    lexer_destroy(lexer);
    free(source);
    
    return lexer->error->has_error ? 1 : 0;
}

// Debug: Show AST
static int debug_parse(const char* input_file) {
    if (!file_exists(input_file)) {
        fprintf(stderr, "Error: File '%s' not found.\n", input_file);
        return 1;
    }
    
    char* source = read_file(input_file);
    if (!source) {
        fprintf(stderr, "Error: Failed to read file '%s'.\n", input_file);
        return 1;
    }
    
    // Tokenize
    Lexer* lexer = lexer_create(source);
    TokenList* tokens = lexer_tokenize(lexer);
    
    if (lexer->error->has_error) {
        fprintf(stderr, "Lexer error: %s\n", lexer->error->message);
        token_list_destroy(tokens);
        lexer_destroy(lexer);
        free(source);
        return 1;
    }
    
    // Parse
    printf("=== PARSING ===\n");
    
    Parser* parser = parser_create(tokens);
    Program* prog = parser_parse(parser);
    
    if (parser->error->has_error) {
        fprintf(stderr, "Parser error: %s\n", parser->error->message);
        parser_destroy(parser);
        token_list_destroy(tokens);
        lexer_destroy(lexer);
        free(source);
        return 1;
    }
    
    // Print AST
    ast_print(prog);
    
    // Clean up
    program_destroy(prog);
    parser_destroy(parser);
    
    token_list_destroy(tokens);
    lexer_destroy(lexer);
    free(source);
    
    return 0;
}

// Compile Aurora source to WASM
static int compile_file(const char* input_file, const char* output_file, bool verbose) {
    if (!file_exists(input_file)) {
        fprintf(stderr, "Error: Source file '%s' not found.\n", input_file);
        return 1;
    }
    
    // Default output name
    char default_output[256];
    if (!output_file) {
        // Remove .aur extension and add .wasm
        strncpy(default_output, input_file, sizeof(default_output) - 1);
        char* dot = strrchr(default_output, '.');
        if (dot) *dot = '\0';
        strcat(default_output, ".wasm");
        output_file = default_output;
    }
    
    if (verbose) {
        printf("Compiling '%s' to '%s'...\n", input_file, output_file);
    }
    
    char* source = read_file(input_file);
    if (!source) {
        fprintf(stderr, "Error: Failed to read file '%s'.\n", input_file);
        return 1;
    }
    
    // Tokenize
    if (verbose) printf("Lexing source code...\n");
    Lexer* lexer = lexer_create(source);
    TokenList* tokens = lexer_tokenize(lexer);
    
    if (lexer->error->has_error) {
        fprintf(stderr, "Lexer error: %s\n", lexer->error->message);
        token_list_destroy(tokens);
        lexer_destroy(lexer);
        free(source);
        return 1;
    }
    
    if (verbose) printf("Parsed %zu tokens.\n", tokens->size);
    
    // Parse to AST
    if (verbose) printf("Parsing to AST...\n");
    Parser* parser = parser_create(tokens);
    Program* prog = parser_parse(parser);
    
    if (parser->error->has_error) {
        fprintf(stderr, "Parser error: %s\n", parser->error->message);
        parser_destroy(parser);
        token_list_destroy(tokens);
        lexer_destroy(lexer);
        free(source);
        return 1;
    }
    
    // Generate WASM
    if (verbose) printf("Generating WebAssembly...\n");
    CodegenContext* codegen = codegen_create();
    char* wat_output = codegen_generate(codegen, prog);
    
    // Write WAT file
    char wat_file[512];
    snprintf(wat_file, sizeof(wat_file), "%s.wat", output_file);
    FILE* wat_fp = fopen(wat_file, "w");
    if (!wat_fp) {
        fprintf(stderr, "Error: Failed to create WAT file '%s'.\n", wat_file);
        free(wat_output);
        codegen_destroy(codegen);
        program_destroy(prog);
        parser_destroy(parser);
        token_list_destroy(tokens);
        lexer_destroy(lexer);
        free(source);
        return 1;
    }
    
    fprintf(wat_fp, "%s", wat_output);
    fclose(wat_fp);
    
    if (verbose) printf("Generated WAT file: %s\n", wat_file);
    
    // Convert WAT to WASM using wat2wasm
    char command[1024];
    snprintf(command, sizeof(command), "wat2wasm %s -o %s", wat_file, output_file);
    
    if (verbose) printf("Running: %s\n", command);
    
    int result = system(command);
    if (result != 0) {
        fprintf(stderr, "Error: wat2wasm failed. Make sure WABT is installed.\n");
        fprintf(stderr, "Install with: sudo apt install wabt\n");
        free(wat_output);
        codegen_destroy(codegen);
        program_destroy(prog);
        parser_destroy(parser);
        token_list_destroy(tokens);
        lexer_destroy(lexer);
        free(source);
        return 1;
    }
    
    // Clean up WAT file unless verbose
    if (!verbose) {
        remove(wat_file);
    }
    
    // Get file size
    struct stat st;
    if (stat(output_file, &st) == 0) {
        printf("✓ Successfully compiled '%s' to '%s' (%ld bytes)\n", 
               input_file, output_file, st.st_size);
    }
    
    // Clean up
    free(wat_output);
    codegen_destroy(codegen);
    program_destroy(prog);
    parser_destroy(parser);
    token_list_destroy(tokens);
    lexer_destroy(lexer);
    free(source);
    
    return 0;
}

// Compile and run Aurora source
static int run_file(const char* input_file, bool verbose) {
    // Create temporary output file
    char temp_wasm[] = "/tmp/aurora_XXXXXX.wasm";
    
    // Compile
    int result = compile_file(input_file, temp_wasm, verbose);
    if (result != 0) {
        return result;
    }
    
    // Run with wasmtime
    if (verbose) printf("Running WASM with wasmtime...\n");
    
    char command[512];
    snprintf(command, sizeof(command), "wasmtime %s", temp_wasm);
    
    result = system(command);
    
    // Clean up temp file
    remove(temp_wasm);
    
    return result;
}

// Main entry point
int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage();
        return 0;
    }
    
    // Check for verbose flag
    bool verbose = false;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = true;
            // Shift arguments
            for (int j = i; j < argc - 1; j++) {
                argv[j] = argv[j + 1];
            }
            argc--;
            i--;
        }
    }
    
    const char* command = argv[1];
    
    // Handle commands
    if (strcmp(command, "version") == 0) {
        print_version();
        return 0;
    }
    
    if (strcmp(command, "build") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: No source file specified.\n");
            fprintf(stderr, "Usage: aurora build <file.aur> [output.wasm]\n");
            return 1;
        }
        
        const char* input_file = argv[2];
        const char* output_file = argc > 3 ? argv[3] : NULL;
        
        return compile_file(input_file, output_file, verbose);
    }
    
    if (strcmp(command, "run") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: No source file specified.\n");
            fprintf(stderr, "Usage: aurora run <file.aur>\n");
            return 1;
        }
        
        return run_file(argv[2], verbose);
    }
    
    if (strcmp(command, "lex") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: No source file specified.\n");
            fprintf(stderr, "Usage: aurora lex <file.aur>\n");
            return 1;
        }
        
        return debug_lex(argv[2]);
    }
    
    if (strcmp(command, "parse") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: No source file specified.\n");
            fprintf(stderr, "Usage: aurora parse <file.aur>\n");
            return 1;
        }
        
        return debug_parse(argv[2]);
    }
    
    if (strcmp(command, "wat") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: No source file specified.\n");
            fprintf(stderr, "Usage: aurora wat <file.aur>\n");
            return 1;
        }
        
        // Generate WAT and print to stdout
        char* source = read_file(argv[2]);
        if (!source) {
            fprintf(stderr, "Error: Failed to read file '%s'.\n", argv[2]);
            return 1;
        }
        
        Lexer* lexer = lexer_create(source);
        TokenList* tokens = lexer_tokenize(lexer);
        
        if (lexer->error->has_error) {
            fprintf(stderr, "Lexer error: %s\n", lexer->error->message);
            token_list_destroy(tokens);
            lexer_destroy(lexer);
            free(source);
            return 1;
        }
        
        Parser* parser = parser_create(tokens);
        Program* prog = parser_parse(parser);
        
        if (parser->error->has_error) {
            fprintf(stderr, "Parser error: %s\n", parser->error->message);
            parser_destroy(parser);
            token_list_destroy(tokens);
            lexer_destroy(lexer);
            free(source);
            return 1;
        }
        
        CodegenContext* codegen = codegen_create();
        char* wat_output = codegen_generate(codegen, prog);
        
        printf("%s", wat_output);
        
        free(wat_output);
        codegen_destroy(codegen);
        program_destroy(prog);
        parser_destroy(parser);
        token_list_destroy(tokens);
        lexer_destroy(lexer);
        free(source);
        
        return 0;
    }
    
    fprintf(stderr, "Error: Unknown command '%s'\n", command);
    fprintf(stderr, "Run 'aurora' without arguments to see usage.\n");
    return 1;
}