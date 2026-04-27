#include "platform/app.h"
#include "ui/controls/button.h"
#include "ui/controls/column.h"
#include "ui/controls/label.h"
#include "ui/root_view_host.h"
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>

int main() {
    std::setbuf(stdout, nullptr);

    auto app = platform::App::create({.renderer_backend = platform::RendererBackend::kOpenGL});
    if (!app) std::abort();

    auto root = std::make_unique<ui::Column>();
    ui::Label* label = root->add_child(std::make_unique<ui::Label>("Button has not been clicked"));
    ui::Button* button = root->add_child(std::make_unique<ui::Button>("Click"));

    int click_count = 0;
    button->on_click([label, &click_count] {
        ++click_count;
        label->set_text("Clicked " + std::to_string(click_count));
    });

    ui::RootViewHost host(std::move(root));
    platform::Window* window =
        app->create_window({.width = 640, .height = 420, .title = "UI Button Demo"}, &host);
    if (!window) std::abort();

    return app->run();
}
