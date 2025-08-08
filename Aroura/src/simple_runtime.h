#ifndef SIMPLE_RUNTIME_H
#define SIMPLE_RUNTIME_H

// Function prototypes
int run_wasm_with_runtime(const char* wasm_file);
int run_wasm_with_wat_wrapper(const char* wasm_file);
int run_wasm_file(const char* wasm_file);

#endif // SIMPLE_RUNTIME_H
