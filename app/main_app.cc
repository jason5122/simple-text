#include "ui/app/app.h"
#include <glad/glad.h>
#include <iostream>

class SimpleText : public App {
public:
    void onActivate() {
        createNewWindow();
        createNewWindow();
    }

    void onOpenGLActivate() {
        std::cerr << "hi\n";

        glEnable(GL_BLEND);
        glDepthMask(GL_FALSE);

        glClearColor(1.0, 0.0, 0.0, 1.0);
    }

    void onDraw() {
        std::cerr << "redraw\n";

        glClear(GL_COLOR_BUFFER_BIT);
    }
};

int SimpleTextMain(int argc, char* argv[]) {
    SimpleText app;
    app.run();
    return 0;
}
