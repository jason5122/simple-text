#include "ui/gtk/editor_window.h"

int SimpleTextMain(int argc, char* argv[]) {
    EditorWindow window;
    int status = window.run();
    return status;
}
