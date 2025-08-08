#ifndef RUNTIME_H
#define RUNTIME_H

#include <wasmtime.h>

// Runtime environment for Aurora WASM modules
typedef struct AuroraRuntime AuroraRuntime;

// Function prototypes
AuroraRuntime* runtime_create(void);
void runtime_destroy(AuroraRuntime* runtime);
int runtime_load_module(AuroraRuntime* runtime, const char* wasm_file);
int runtime_run_main(AuroraRuntime* runtime);
int run_wasm_file(const char* wasm_file);

#endif // RUNTIME_H
