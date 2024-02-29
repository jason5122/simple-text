#pragma once

#include "ui/renderer/rect_renderer.h"
#include <QBasicTimer>
#include <QMatrix4x4>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLWidget>
#include <QQuaternion>
#include <QVector2D>

class GeometryEngine;

class MainWidget : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

public:
    using QOpenGLWidget::QOpenGLWidget;

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

private:
    QBasicTimer timer;
    QOpenGLShaderProgram program;

    QMatrix4x4 projection;

    QVector2D mousePressPosition;
    QVector3D rotationAxis;
    qreal angularSpeed = 0;
    QQuaternion rotation;

    RectRenderer rect_renderer;
};
