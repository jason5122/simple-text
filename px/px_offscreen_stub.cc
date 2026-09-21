#include "px/px_offscreen.h"

// Platforms whose backend has no offscreen path yet. Callers are expected to handle a null
// surface, so a headless capture reports that it is unavailable rather than failing to link.

px_offscreen_t* px_create_offscreen(double width, double height, double dpi_scale) {
    return nullptr;
}

void px_destroy_offscreen(px_offscreen_t* surface) {}

bool px_offscreen_paint(px_offscreen_t* surface, px_window_event_handler* handler) {
    return false;
}

const uint32_t* px_offscreen_pixels(const px_offscreen_t* surface) { return nullptr; }

int px_offscreen_width(const px_offscreen_t* surface) { return 0; }

int px_offscreen_height(const px_offscreen_t* surface) { return 0; }
