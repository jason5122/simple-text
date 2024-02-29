#include "editor_window_qt.h"
#include "ui/qt/main_widget.h"
#include <QSurfaceFormat>

EditorWindowQt::EditorWindowQt(int argc, char* argv[]) : app(argc, argv) {}

int EditorWindowQt::run() {
    QSurfaceFormat format;
    // format.setDepthBufferSize(24);
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    // format.setRenderableType(QSurfaceFormat::OpenGL);
    QSurfaceFormat::setDefaultFormat(format);

    app.setApplicationName("cube");
    app.setApplicationVersion("0.1");
    MainWidget widget;
    widget.show();
    return app.exec();
}
