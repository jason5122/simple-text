#include "simple_text.h"
#include <memory>

SimpleText::SimpleText() {}

SimpleText::~SimpleText() {}

void SimpleText::onLaunch() {
    createWindow();
}

void SimpleText::createWindow() {
    std::unique_ptr<EditorWindow> editor_window =
        std::make_unique<EditorWindow>(*this, 600, 400, editor_windows.size());
    editor_window->show();
    editor_windows.push_back(std::move(editor_window));
}

void SimpleText::destroyWindow(int wid) {
    editor_windows[wid] = nullptr;
}

void SimpleText::createNWindows(int n) {
    for (int i = 0; i < n; i++) {
        createWindow();
    }
}

void SimpleText::destroyAllWindows() {
    for (const auto& editor_window : editor_windows) {
        if (editor_window != nullptr) {
            editor_window->close();
        }
    }

    createWindow();
}
