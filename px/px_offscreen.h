#pragma once

#include "px/px.h"
#include <cstdint>

// A windowless render target. It drives the same paint path a window backend drives -- the same
// render context, the same dirty-rectangle handling, the same glyph atlases -- into a texture we
// own rather than a drawable the window server composites. What comes back is the renderer's
// output, not a screenshot of it, so a capture needs no run loop, no focus and no display.
//
// Implemented on Metal. The other backends return null until they grow one.
struct px_offscreen_t;

// `width` and `height` are logical, as in px_create_window; the surface holds width * dpi_scale by
// height * dpi_scale device pixels. Null when the platform has no offscreen path or no GPU.
px_offscreen_t* px_create_offscreen(double width, double height, double dpi_scale);
void px_destroy_offscreen(px_offscreen_t* surface);

// Paints the whole surface through `handler` and reads the result back, blocking until the GPU is
// done. False if the frame could not be recorded or completed, in which case the previously read
// pixels are left alone.
bool px_offscreen_paint(px_offscreen_t* surface, px_window_event_handler* handler);

// Tightly packed premultiplied BGRA device pixels from the most recent successful paint, first row
// at the top. Valid until the next paint or until the surface is destroyed; null before the first
// successful paint.
const uint32_t* px_offscreen_pixels(const px_offscreen_t* surface);
int px_offscreen_width(const px_offscreen_t* surface);
int px_offscreen_height(const px_offscreen_t* surface);
