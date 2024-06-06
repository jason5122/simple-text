#include "build/buildflag.h"
#include "simple_text.h"
#include <memory>

#include <iostream>

void SimpleText::onLaunch() {
    createWindow();
}

void SimpleText::onQuit() {
    std::cerr << "SimpleText::onQuit()\n";
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
