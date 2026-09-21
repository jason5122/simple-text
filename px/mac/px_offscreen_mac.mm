#include "px/px_offscreen.h"

#include "px/mac/px_offscreen_mac.h"
#include <cstdlib>

struct px_offscreen_t {
    std::unique_ptr<px_mac_offscreen> backend;
};

px_offscreen_t* px_create_offscreen(double width, double height, double dpi_scale) {
    std::unique_ptr<px_mac_offscreen> backend =
        std::getenv("PX_GL") != nullptr ? px_create_gl_offscreen(width, height, dpi_scale)
                                        : px_create_metal_offscreen(width, height, dpi_scale);
    if (!backend) {
        return nullptr;
    }
    return new px_offscreen_t{.backend = std::move(backend)};
}

void px_destroy_offscreen(px_offscreen_t* surface) { delete surface; }

bool px_offscreen_paint(px_offscreen_t* surface, px_window_event_handler* handler) {
    return surface && handler && surface->backend->paint(handler);
}

const uint32_t* px_offscreen_pixels(const px_offscreen_t* surface) {
    return surface ? surface->backend->pixels() : nullptr;
}

int px_offscreen_width(const px_offscreen_t* surface) {
    return surface ? surface->backend->width() : 0;
}

int px_offscreen_height(const px_offscreen_t* surface) {
    return surface ? surface->backend->height() : 0;
}
