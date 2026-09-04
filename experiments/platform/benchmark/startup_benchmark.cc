// End-to-end process-start-to-first-paint benchmark for the px/ui window path.

#include <cstdio>
#include <cstdlib>

#include "experiments/platform/ui/window.h"

namespace {

constexpr double kWindowWidth = 1200.0;
constexpr double kWindowHeight = 800.0;
constexpr fcolor kBackground{0.0f, 0.0f, 0.0f, 1.0f};

class StartupWindow final : public window_impl {
public:
    StartupWindow()
        : window_impl(kWindowWidth, kWindowHeight, "px/ui startup benchmark", kBackground) {}

    void paint(px_render_context* context,
               rect bounds,
               const rect* dirty,
               int dirty_count) override {
        std::puts("draw");

        // The GL paint callback may run off the main thread, so bypass platform teardown. This
        // keeps the terminal measurement point at entry to the application's first paint.
        std::_Exit(EXIT_SUCCESS);
    }
};

}  // namespace

int main(int argc, char** argv) {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    px_init("px/ui startup benchmark", "com.example.px-ui-startup-benchmark", argc, argv, 0);

    StartupWindow window;
    window.show();
    window.mark_dirty();
    px_run_event_loop();
    return 0;
}
