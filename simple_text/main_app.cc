#include "simple_text/editor_window.h"
#include "ui/app/app.h"
#include <glad/glad.h>
#include <iostream>

class SimpleText : public App {
private:
    std::vector<std::unique_ptr<EditorWindow>> windows;

    void onActivateVirtual() {
        createWindow(600, 400);
    }

    // TODO: Ensure this is thread safe!
    void createWindowVirtual(int width, int height) {
        // std::unique_ptr<EditorWindow> window =
        //     std::make_unique<EditorWindow>(*this, windows.size());
        // window->createWithSize(width, height);
        // windows.push_back(std::move(window));

        windows.emplace_back(std::make_unique<EditorWindow>(*this, windows.size()));
        windows.back()->createWithSize(width, height);
    }

    // TODO: Implement this in a better way.
    void destroyWindowVirtual(int idx) {
        std::cerr << "start\n";

        // windows[idx]->close();
        // windows[idx].reset();

        // windows.back()->close();
        windows.back().reset();
        std::cerr << "in between\n";
        windows.pop_back();

        std::cerr << "windows size: " << windows.size() << '\n';
    }
};

int SimpleTextMain(int argc, char* argv[]) {
    SimpleText editor;
    editor.run();
    return 0;
}
