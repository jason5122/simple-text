#pragma once

#include <QtWidgets>

class EditorWindowQt {
public:
    EditorWindowQt(int argc, char* argv[]);
    int run();

private:
    QApplication app;
};
