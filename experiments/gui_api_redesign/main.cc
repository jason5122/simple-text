#include <iostream>

#include "experiments/gui_api_redesign/app.h"

int main() {
    std::cout << "hello\n";

    App app;

    Window& main_window = app.create_window(800, 600);
    main_window.on_draw([]() {});

    return app.run();
}
