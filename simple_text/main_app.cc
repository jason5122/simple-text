#include "simple_text/simple_text.h"

#include <iostream>

int SimpleTextMain(int argc, char* argv[]) {
    SimpleText simple_text;
    std::cerr << simple_text.editor_windows.size() << '\n';
    simple_text.run();
    return 0;
}
