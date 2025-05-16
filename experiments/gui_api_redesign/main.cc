#include "experiments/gui_api_redesign/app.h"

int main() {
    App app;

    Window& main_window = app.create_window(800, 600);
    main_window.on_draw([]() {});

    return app.run();
}
