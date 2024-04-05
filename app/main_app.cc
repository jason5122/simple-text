#include "ui/app/app.h"
#include "util/profile_util.h"
#include <glad/glad.h>
#include <iostream>

class SimpleText : public App {
public:
    void onActivate() {
        createNewWindow();
        createNewWindow();
    }

    void onOpenGLActivate() {
        std::cerr << glGetString(GL_VERSION) << '\n';

        glEnable(GL_BLEND);
        glDepthMask(GL_FALSE);

        glClearColor(1.0, 0.0, 0.0, 1.0);
    }

    void onDraw() {
        {
            PROFILE_BLOCK("redraw");
            glClear(GL_COLOR_BUFFER_BIT);
        }
    }
};

int SimpleTextMain(int argc, char* argv[]) {
    SimpleText app;
    app.run();
    return 0;
}
