#include "simple_text/editor_window.h"
#include "ui/app/app.h"

class SimpleText : public Parent {
private:
    void onActivate() override {
        createChild();
    }

    void createChild() override {
        EditorWindow* editor_window = new EditorWindow(*this, 600, 400);
        editor_window->show();
    }

    // Object slicing occurs here, preventing ~EditorWindow() from being called.
    // We need to cast back to EditorWindow*.
    void destroyChild(Child* child) override {
        if (child != nullptr) {
            EditorWindow* editor_window = static_cast<EditorWindow*>(child);
            editor_window->close();
            delete editor_window;
        }
    }
};

int SimpleTextMain(int argc, char* argv[]) {
    SimpleText simple_text;
    simple_text.run();
    return 0;
}
