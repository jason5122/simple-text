#include "ui/app/app.h"

class SimpleText : public App {
public:
    void onActivate() {
        createNewWindow();
    }
};

int SimpleTextMain(int argc, char* argv[]) {
    SimpleText app;
    app.run();
    return 0;
}
