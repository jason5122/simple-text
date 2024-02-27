#include "ui/qt/editor_window_qt.h"
#include <string>

int SimpleTextMain() {
    int argc = 1;
    EditorWindowQt window(argc);
    return window.run();
}
