#include "app.h"

#include "opengl/functions_gl.h"

// TODO: Debug; remove this.
#include <fmt/base.h>
#include <fmt/format.h>

namespace gui {

// We should have an OpenGL context within this function.
// Load OpenGL function pointers and perform OpenGL setup here.
void FastStartupApp::on_launch() {
    opengl::FunctionsGL functions_gl{};
    functions_gl.load_global_function_pointers();

    create_window();
}

void FastStartupApp::create_window() {
    auto window = std::make_unique<FastStartupWindow>(*this, 1200, 800, windows.size());

#ifdef NDEBUG
    const std::string& debug_or_release = "Release";
#else
    const std::string& debug_or_release = "Debug";
#endif
    window->set_title(fmt::format("Fast Startup Experiment ({})", debug_or_release));

    window->show();
    windows.emplace_back(std::move(window));
}

void FastStartupApp::destroy_window(int wid) { windows[wid] = nullptr; }

}  // namespace gui
