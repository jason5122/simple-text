#include "main_widget.h"
#include <iostream>

void MainWidget::initializeGL() {
    glDisable(GL_DEPTH_TEST);
    glClearColor(253 / 255.0, 253 / 255.0, 253 / 255.0, 1.0);

    float scaled_width = width() * devicePixelRatio();
    float scaled_height = height() * devicePixelRatio();

    rect_renderer.setup(scaled_width, scaled_height);
}

void MainWidget::resizeGL(int w, int h) {}

void MainWidget::paintGL() {
    float scaled_width = width() * devicePixelRatio();
    float scaled_height = height() * devicePixelRatio();

    glClear(GL_COLOR_BUFFER_BIT);

    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE);
    rect_renderer.resize(scaled_width, scaled_height);
    rect_renderer.draw(0, 0, 0, 0, 0, 80, 500, 400, 60);
}
