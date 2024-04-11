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

    void destroyChildVirtual(Child* child) {
        if (!child) return;

        child->destroy();
        delete child;
    }
};

int SimpleTextMain(int argc, char* argv[]) {
    SimpleText simple_text;
    simple_text.run();
    return 0;
}
