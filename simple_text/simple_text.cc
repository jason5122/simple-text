#include "build/buildflag.h"
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
    // FIXME: Why does this crash when iterating over *all* windows?
    for (size_t i = editor_windows.size() - 1; i >= 1; i--) {
        if (editor_windows[i]) {
            editor_windows[i]->close();
        }
    }
}
