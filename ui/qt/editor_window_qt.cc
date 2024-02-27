#include "editor_window_qt.h"

EditorWindowQt::EditorWindowQt(int& argc) : app(argc, nullptr) {}

int EditorWindowQt::run() {
    QWidget window;
    window.resize(320, 240);
    window.show();
    window.setWindowTitle(QApplication::translate("toplevel", "Top-level widget"));

    return app.exec();
}
