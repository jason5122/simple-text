#include "simple_text.h"
#include "simple_text/editor_window.h"

void SimpleText::onLaunch() {
    createChild(0, 1000);
}

void SimpleText::createChild(int x, int y) {
    EditorWindow* editor_window = new EditorWindow(*this, x, y, 600, 400);
    editor_window->show();

    // TODO: Debug; remove this.
    incrementWindowCount();
}

void SimpleText::destroyChild(EditorWindow* editor_window) {
    editor_window->close();
    delete editor_window;
}
