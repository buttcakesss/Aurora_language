#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wasmtime.h>

// Runtime environment for Aurora WASM modules
typedef struct {
    wasmtime_store_t *store;
    wasmtime_context_t *context;
    wasmtime_instance_t instance;
    wasmtime_func_t print_i32_func;
    wasmtime_func_t print_string_func;
} AuroraRuntime;

// Print function for integers
static wasm_trap_t* print_i32(void *env, wasmtime_caller_t *caller, const wasmtime_val_t *args, wasmtime_val_t *results) {
    (void)env;
    (void)caller;
    
    if (args[0].kind == WASMTIME_I32) {
        printf("%d", args[0].of.i32);
    }
    return NULL;
}

// Print function for strings (if needed)
static wasm_trap_t* print_string(void *env, wasmtime_caller_t *caller, const wasmtime_val_t *args, wasmtime_val_t *results) {
    (void)env;
    (void)caller;
    
    if (args[0].kind == WASMTIME_I32) {
        // For now, just print the integer as a character
        printf("%c", args[0].of.i32);
    }
    return NULL;
}

// Create runtime environment
AuroraRuntime* runtime_create(void) {
    AuroraRuntime* runtime = malloc(sizeof(AuroraRuntime));
    if (!runtime) return NULL;
    
    // Initialize Wasmtime
    wasm_config_t *config = wasm_config_new();
    wasmtime_engine_t *engine = wasmtime_engine_new(config);
    wasmtime_store_t *store = wasmtime_store_new(engine, NULL, NULL);
    wasmtime_context_t *context = wasmtime_store_context(store);
    
    runtime->store = store;
    runtime->context = context;
    
    return runtime;
}

// Destroy runtime environment
void runtime_destroy(AuroraRuntime* runtime) {
    if (!runtime) return;
    
    wasmtime_store_delete(runtime->store);
    free(runtime);
}

// Load and instantiate WASM module
int runtime_load_module(AuroraRuntime* runtime, const char* wasm_file) {
    // Read WASM file
    FILE* file = fopen(wasm_file, "rb");
    if (!file) {
        fprintf(stderr, "Error: Could not open WASM file '%s'\n", wasm_file);
        return 1;
    }
    
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char* wasm_bytes = malloc(size);
    if (!wasm_bytes) {
        fclose(file);
        return 1;
    }
    
    fread(wasm_bytes, 1, size, file);
    fclose(file);
    
    // Create WASM module
    wasm_byte_vec_t wasm = {size, wasm_bytes};
    wasmtime_module_t *module = wasmtime_module_new(runtime->store, &wasm, NULL);
    free(wasm_bytes);
    
    if (!module) {
        fprintf(stderr, "Error: Failed to create WASM module\n");
        return 1;
    }
    
    // Create imports
    wasmtime_extern_t imports[2];
    
    // Import print_i32 function
    wasmtime_functype_t *print_i32_type = wasmtime_functype_new_1_0(
        wasm_valtype_new_i32(), NULL);
    wasmtime_func_t print_i32_func = wasmtime_func_new(runtime->store, print_i32_type, print_i32, NULL, NULL);
    imports[0] = wasmtime_extern_func(print_i32_func);
    
    // Import print_string function (if needed)
    wasmtime_functype_t *print_string_type = wasmtime_functype_new_1_0(
        wasm_valtype_new_i32(), NULL);
    wasmtime_func_t print_string_func = wasmtime_func_new(runtime->store, print_string_type, print_string, NULL, NULL);
    imports[1] = wasmtime_extern_func(print_string_func);
    
    // Create import object
    wasmtime_extern_vec_t import_list = {2, imports};
    
    // Instantiate module
    wasmtime_instance_t instance;
    wasm_trap_t *trap = wasmtime_instance_new(runtime->store, module, &import_list, &instance, NULL);
    
    if (trap) {
        fprintf(stderr, "Error: Failed to instantiate WASM module\n");
        wasmtime_trap_delete(trap);
        wasmtime_module_delete(module);
        return 1;
    }
    
    runtime->instance = instance;
    runtime->print_i32_func = print_i32_func;
    runtime->print_string_func = print_string_func;
    
    wasmtime_module_delete(module);
    return 0;
}

// Run the main function
int runtime_run_main(AuroraRuntime* runtime) {
    // Look for main function
    wasmtime_extern_t main_func;
    bool found = wasmtime_instance_export_get(runtime->store, &runtime->instance, "main", &main_func);
    
    if (!found || main_func.kind != WASMTIME_EXTERN_FUNC) {
        fprintf(stderr, "Warning: No main function found in WASM module\n");
        return 0;
    }
    
    // Call main function
    wasmtime_val_t results[1];
    wasm_trap_t *trap = wasmtime_func_call(runtime->store, &main_func.of.func, NULL, 0, results, 0, NULL);
    
    if (trap) {
        fprintf(stderr, "Error: Trap occurred while running main function\n");
        wasmtime_trap_delete(trap);
        return 1;
    }
    
    return 0;
}

// Simple runtime runner
int run_wasm_file(const char* wasm_file) {
    AuroraRuntime* runtime = runtime_create();
    if (!runtime) {
        fprintf(stderr, "Error: Failed to create runtime\n");
        return 1;
    }
    
    int result = runtime_load_module(runtime, wasm_file);
    if (result == 0) {
        result = runtime_run_main(runtime);
    }
    
    runtime_destroy(runtime);
    return result;
}
