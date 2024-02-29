#include "editor_window_qt.h"
#include <QApplication>
#include <QLabel>
#include <QSurfaceFormat>

#ifndef QT_NO_OPENGL
#include "mainwidget.h"
#endif

EditorWindowQt::EditorWindowQt(int argc, char* argv[]) : app(argc, argv) {}

int EditorWindowQt::run() {
    // QWidget window;
    // window.resize(320, 240);
    // window.show();
    // window.setWindowTitle(QApplication::translate("toplevel", "Top-level widget"));
    // return app.exec();

    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    QSurfaceFormat::setDefaultFormat(format);

    app.setApplicationName("cube");
    app.setApplicationVersion("0.1");
#ifndef QT_NO_OPENGL
    MainWidget widget;
    widget.show();
#else
    QLabel note("OpenGL Support required");
    note.show();
#endif
    return app.exec();
}
