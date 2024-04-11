#include "simple_text/editor_window.h"
#include "ui/app/app.h"

class SimpleText : public Parent {
private:
    void onActivateVirtual() {
        EditorWindow* editor_window = new EditorWindow(*this);
        editor_window->create(600, 400);
    }
};

int SimpleTextMain(int argc, char* argv[]) {
    SimpleText simple_text;
    simple_text.run();
    return 0;
}
