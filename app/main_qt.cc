#include "ui/qt/editor_window_qt.h"
#include <string>

int SimpleTextMain(int argc, char* argv[]) {
    EditorWindowQt window(argc, argv);
    return window.run();
}
