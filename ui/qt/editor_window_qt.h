#pragma once

#include <QtWidgets>

class EditorWindowQt {
public:
    EditorWindowQt(int& argc);
    int run();

private:
    QApplication app;
};
