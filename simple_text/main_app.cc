#include "ui/app/app.h"

class SimpleText : public Parent {};

int SimpleTextMain(int argc, char* argv[]) {
    SimpleText simple_text;
    simple_text.run();
    return 0;
}
