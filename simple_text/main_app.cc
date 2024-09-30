#include "simple_text/editor_app.h"

extern "C" void hello_from_rust();

int SimpleTextMain(int argc, char* argv[]) {
    hello_from_rust();

    EditorApp editor_app;
    editor_app.run();
    return 0;
}
