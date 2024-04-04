#include "ui/app/app.h"
#include "ui/window/editor_window.h"
#include <iostream>

class SimpleText : public App {
public:
    void onActivate() {
        editor_window.show();
        std::cerr << "hi\n";
    }

private:
    EditorWindow2 editor_window;
};

int SimpleTextMain(int argc, char* argv[]) {
    SimpleText app;
    app.run();
    return 0;
}
