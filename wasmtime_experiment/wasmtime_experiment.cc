#include "base/filesystem/file_reader.h"
#include "wasmtime_experiment.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <wasm.h>
#include <wasmtime.h>

// TODO: Debug use; remove this.
#include "util/std_print.h"

static void exit_with_error(const char* message, wasmtime_error_t* error, wasm_trap_t* trap);

static wasm_trap_t* hello_callback(void* env,
                                   wasmtime_caller_t* caller,
                                   const wasmtime_val_t* args,
                                   size_t nargs,
                                   wasmtime_val_t* results,
                                   size_t nresults) {
    std::println("Calling back...");
    std::println("> Hello World!");
    return NULL;
}

void wasmtime_experiment() {
    // Set up our compilation context. Note that we could also work with a
    // `wasm_config_t` here to configure what feature are enabled and various
    // compilation settings.
    std::println("Initializing...");
    wasm_engine_t* engine = wasm_engine_new();
    assert(engine != NULL);

    // With an engine we can create a *store* which is a long-lived group of wasm
    // modules. Note that we allocate some custom data here to live in the store,
    // but here we skip that and specify NULL.
    wasmtime_store_t* store = wasmtime_store_new(engine, NULL, NULL);
    assert(store != NULL);
    wasmtime_context_t* context = wasmtime_store_context(store);

    // Read our input file, which in this case is a wasm text file.
    fs::path wasm_path = base::ResourceDir() / "wasm/hello.wat";
    FILE* file = fopen(wasm_path.c_str(), "r");
    assert(file != NULL);
    fseek(file, 0L, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0L, SEEK_SET);
    wasm_byte_vec_t wat;
    wasm_byte_vec_new_uninitialized(&wat, file_size);
    if (fread(wat.data, file_size, 1, file) != 1) {
        std::println("> Error loading module!");
        return;
    }
    fclose(file);

    // Parse the wat into the binary wasm format
    wasm_byte_vec_t wasm;
    wasmtime_error_t* error = wasmtime_wat2wasm(wat.data, wat.size, &wasm);
    if (error != NULL) exit_with_error("failed to parse wat", error, NULL);
    wasm_byte_vec_delete(&wat);

    // Now that we've got our binary webassembly we can compile our module.
    std::println("Compiling module...");
    wasmtime_module_t* module = NULL;
    error = wasmtime_module_new(engine, (uint8_t*)wasm.data, wasm.size, &module);
    wasm_byte_vec_delete(&wasm);
    if (error != NULL) exit_with_error("failed to compile module", error, NULL);

    // Next up we need to create the function that the wasm module imports. Here
    // we'll be hooking up a thunk function to the `hello_callback` native
    // function above. Note that we can assign custom data, but we just use NULL
    // for now).
    std::println("Creating callback...");
    wasm_functype_t* hello_ty = wasm_functype_new_0_0();
    wasmtime_func_t hello;
    wasmtime_func_new(context, hello_ty, hello_callback, NULL, NULL, &hello);

    // With our callback function we can now instantiate the compiled module,
    // giving us an instance we can then execute exports from. Note that
    // instantiation can trap due to execution of the `start` function, so we need
    // to handle that here too.
    std::println("Instantiating module...");
    wasm_trap_t* trap = NULL;
    wasmtime_instance_t instance;
    wasmtime_extern_t import;
    import.kind = WASMTIME_EXTERN_FUNC;
    import.of.func = hello;
    error = wasmtime_instance_new(context, module, &import, 1, &instance, &trap);
    if (error != NULL || trap != NULL) exit_with_error("failed to instantiate", error, trap);

    // Lookup our `run` export function
    std::println("Extracting export...");
    wasmtime_extern_t run;
    bool ok = wasmtime_instance_export_get(context, &instance, "run", 3, &run);
    assert(ok);
    assert(run.kind == WASMTIME_EXTERN_FUNC);

    // And call it!
    std::println("Calling export...");
    error = wasmtime_func_call(context, &run.of.func, NULL, 0, NULL, 0, &trap);
    if (error != NULL || trap != NULL) exit_with_error("failed to call function", error, trap);

    // Clean up after ourselves at this point
    std::println("All finished!");

    wasmtime_module_delete(module);
    wasmtime_store_delete(store);
    wasm_engine_delete(engine);
    return;
}

static void exit_with_error(const char* message, wasmtime_error_t* error, wasm_trap_t* trap) {
    std::println(stderr, "error: {}\n", message);
    wasm_byte_vec_t error_message;
    if (error != NULL) {
        wasmtime_error_message(error, &error_message);
        wasmtime_error_delete(error);
    } else {
        wasm_trap_message(trap, &error_message);
        wasm_trap_delete(trap);
    }
    std::println(stderr, "{}\n", (int)error_message.size, error_message.data);
    wasm_byte_vec_delete(&error_message);
    exit(1);
}
