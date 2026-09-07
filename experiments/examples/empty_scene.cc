// The smallest px window: one background fill per frame and nothing else.
//
// Everything a resize does apart from painting content is still exercised -- the layer, the
// dirty-rect plumbing, the render context, presentation -- so dragging this corner shows what px
// itself costs. If this is not perfectly smooth, no scene will be. Compare with empty_window,
// which is the same window with no px in it at all.
//
//   ./out/release/empty_scene
//   PX_GL=1 ./out/release/empty_scene

#include "px/px.h"

namespace {

class EmptyScene final : public px_window_event_handler {
public:
    bool handle_event(px_event_t* event) override {
        if (event->type == PX_EVENT_RESIZE) {
            size_ = event->size;
        }
        return false;
    }

    void paint(px_render_context* rc, rect bounds, const rect* dirty, int dirty_count) override {
        rc->draw_rect(rect{0.0, 0.0, size_.x, size_.y}, fcolor{0.97f, 0.97f, 0.97f, 1.0f});
    }

    void set_size(vec2 size) { size_ = size; }

private:
    vec2 size_;
};

}  // namespace

int main(int argc, char** argv) {
    px_init("empty_scene", "com.jason5122.empty-scene", argc, argv, 0);

    EmptyScene scene;
    px_window_t* window = px_create_window(&scene, nullptr, 1200.0, 800.0, "empty scene",
                                           fcolor{0.97f, 0.97f, 0.97f, 1.0f}, PX_WINDOW_DEFAULT);
    scene.set_size(px_window_size(window));
    px_show_window(window);
    px_run_event_loop();
    px_destroy_window(window);
    return 0;
}
