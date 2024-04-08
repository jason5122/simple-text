#include "simple_text/editor_window.h"
#include "ui/app/app.h"
#include <glad/glad.h>

class SimpleText : public App {
public:
    SimpleText() : window1(*this, 0), window2(*this, 1) {}

    void onActivate() {
        window1.createWithSize(1200, 800);
        window2.createWithSize(600, 400);
    }

private:
    EditorWindow window1;
    EditorWindow window2;
};

int SimpleTextMain(int argc, char* argv[]) {
    SimpleText editor;
    editor.run();
    return 0;
}
