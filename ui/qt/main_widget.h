#pragma once

#include "ui/renderer/rect_renderer.h"
// FIXME: Suppress warnings caused by clashes when including <OpenGL/gl3.h> on macOS.
#include <QOpenGLWidget>

// Look into QOpenGLFunctions_3_3_Core:
// https://stackoverflow.com/a/28243012/14698275

class MainWidget : public QOpenGLWidget {
    Q_OBJECT

public:
    using QOpenGLWidget::QOpenGLWidget;

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

private:
    RectRenderer rect_renderer;
};
