#include "ui/app/app.h"
#include "ui/app/app_window.h"
#include "util/profile_util.h"
#include <glad/glad.h>
#include <iostream>

class EditorWindow : public AppWindow {
public:
    EditorWindow(int id) : id(id) {}

    void onOpenGLActivate() {
        std::cerr << "id " << id << ": " << glGetString(GL_VERSION) << '\n';

        glEnable(GL_BLEND);
        glDepthMask(GL_FALSE);

        glClearColor(0.9, 0.9, 0.9, 1.0);
    }

    void onDraw() {
        {
            PROFILE_BLOCK("id " + std::to_string(id) + ": redraw");
            glClear(GL_COLOR_BUFFER_BIT);
        }
    }

private:
    int id;
};

class SimpleText : public App {
public:
    SimpleText() : editor_window1(0), editor_window2(1) {}

    void onActivate() {
        createNewWindow(&editor_window1);
        createNewWindow(&editor_window2);
    }

private:
    EditorWindow editor_window1;
    EditorWindow editor_window2;
};

int SimpleTextMain(int argc, char* argv[]) {
    SimpleText editor;
    editor.run();
    return 0;
}
