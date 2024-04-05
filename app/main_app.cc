#include "ui/app/app.h"
#include "ui/app/win32/main_window.h"
#include <shellscalingapi.h>
#include <windows.h>

class SimpleText : public App {
public:
    void onActivate() {
        // createNewWindow();
        // createNewWindow();
    }
};

int SimpleTextMain(int argc, char* argv[]) {
    SimpleText app;
    app.createNewWindow();
    app.createNewWindow();
    app.run();
    return 0;
}
