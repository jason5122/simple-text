#include "simple_text/editor_window.h"
#include "ui/app/app.h"
#include <glad/glad.h>

class SimpleText : public App {
private:
    int window_id_counter = 0;
    std::vector<std::unique_ptr<EditorWindow>> windows;

    void onActivateVirtual() {
        addWindow(600, 400);
    }

    void addWindowVirtual(int width, int height) {
        std::unique_ptr<EditorWindow> window =
            std::make_unique<EditorWindow>(*this, window_id_counter++);
        window->createWithSize(width, height);
        windows.push_back(std::move(window));
    }
};

int SimpleTextMain(int argc, char* argv[]) {
    SimpleText editor;
    editor.run();
    return 0;
}
