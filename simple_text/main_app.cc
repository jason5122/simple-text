#include "simple_text/editor_window.h"
#include "ui/app/app.h"
#include <glad/glad.h>

class SimpleText : public App {
private:
    std::vector<std::unique_ptr<EditorWindow>> windows;

    void onActivateVirtual() {
        addWindow(600, 400);
    }

    // TODO: Ensure this is thread safe!
    void addWindowVirtual(int width, int height) {
        std::unique_ptr<EditorWindow> window =
            std::make_unique<EditorWindow>(*this, windows.size());
        window->createWithSize(width, height);
        windows.push_back(std::move(window));
    }

    // TODO: Implement this in a better way.
    void removeWindowVirtual(int idx) {
        windows[idx].reset();
    }
};

int SimpleTextMain(int argc, char* argv[]) {
    SimpleText editor;
    editor.run();
    return 0;
}
