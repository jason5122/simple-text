#include "simple_text/editor_window.h"
#include "ui/app/app.h"

class SimpleText : public Parent {
private:
    void onActivateVirtual() {
        createChild();
    }

    void createChildVirtual() {
        EditorWindow* editor_window = new EditorWindow(*this);
        editor_window->create(600, 400);
    }

    // Object slicing occurs here, preventing ~EditorWindow() from being called.
    // We need to cast back to EditorWindow*.
    void destroyChildVirtual(Child* child) {
        if (child != nullptr) {
            EditorWindow* editor_window = static_cast<EditorWindow*>(child);
            editor_window->destroy();
            delete editor_window;
        }
    }
};

int SimpleTextMain(int argc, char* argv[]) {
    SimpleText simple_text;
    simple_text.run();
    return 0;
}
