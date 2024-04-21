#include "simple_text.h"
// #include "simple_text/editor_window.h"
#include <memory>

void SimpleText::onLaunch() {
    createWindow();
}

#include <iostream>

void SimpleText::createWindow() {
    EditorWindow* editor_window = new EditorWindow(*this, 600, 400);
    editor_window->show();
}

void SimpleText::destroyWindow(EditorWindow* editor_window) {
    // FIXME: Make this work properly for GTK.
    // delete editor_window;
    // editor_window->~EditorWindow();
}

void SimpleText::createAllWindows() {
    for (int i = 0; i < 20; i++) {
        // EditorWindow* editor_window = new EditorWindow(*this, 600, 400);
        // editor_window->show();
        // editor_windows.push_back(editor_window);

        std::unique_ptr<EditorWindow> editor_window =
            std::make_unique<EditorWindow>(*this, 600, 400);
        editor_window->show();
        editor_windows_unique.push_back(std::move(editor_window));

        // std::cerr << editor_windows.size() << '\n';
    }
}

void SimpleText::destroyAllWindows() {
    // for (const auto& editor_window : editor_windows) {
    //     editor_window->close();
    // }
    // editor_windows.clear();

    for (const auto& editor_window : editor_windows_unique) {
        editor_window->close();
    }
    editor_windows_unique.clear();
}
