#include "ui/app/app.h"
#include "ui/app/app_window.h"
#include "ui/renderer/image_renderer.h"
#include "ui/renderer/rect_renderer.h"
#include "util/profile_util.h"
#include <glad/glad.h>
#include <iostream>

class EditorWindow : public AppWindow {
public:
    EditorWindow(int id) : id(id) {}

    void onOpenGLActivate(int width, int height) {
        std::cerr << "id " << id << ": " << glGetString(GL_VERSION) << '\n';

        glEnable(GL_BLEND);
        glDepthMask(GL_FALSE);

        glClearColor(0.9, 0.9, 0.9, 1.0);

        rect_renderer.setup(width, height);
        image_renderer.setup(width, height);
    }

    void onDraw() {
        {
            PROFILE_BLOCK("id " + std::to_string(id) + ": redraw");
            glClear(GL_COLOR_BUFFER_BIT);

            glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE);
            rect_renderer.draw(0, 0, 0, 0, 40, 100, 500, 200 * 2, 30 * 2, 40);

            image_renderer.draw(0, 0, 200 * 2, 30 * 2);
        }
    }

    void onResize(int width, int height) {
        glViewport(0, 0, width, height);
        rect_renderer.resize(width, height);
        image_renderer.resize(width, height);
    }

private:
    int id;
    RectRenderer rect_renderer;
    ImageRenderer image_renderer;
};

class SimpleText : public App {
public:
    SimpleText() : editor_window1(0), editor_window2(1) {}

    void onActivate() {
        createNewWindow(editor_window1);
        createNewWindow(editor_window2);
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
