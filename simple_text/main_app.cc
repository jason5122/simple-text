#include "simple_text/editor_app.h"

#include "wasmtime_experiment/wasmtime_experiment.h"
extern "C" void hello_from_rust();

int SimpleTextMain(int argc, char* argv[]) {
    hello_from_rust();
    wasmtime_experiment();

    EditorApp editor_app;
    editor_app.run();
    return 0;
}
