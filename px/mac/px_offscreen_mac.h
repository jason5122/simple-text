#pragma once

#include "px/px.h"
#include <cstdint>
#include <memory>

// macOS has two renderers, so it has two offscreen surfaces behind px_create_offscreen. Metal is
// the default and PX_GL selects OpenGL, the same switch px_create_window reads -- which makes the
// pair useful as a cross-check: both drive the same paint through different pipelines, and the
// captures should agree.
class px_mac_offscreen {
public:
    virtual ~px_mac_offscreen() = default;

    virtual bool paint(px_window_event_handler* handler) = 0;
    virtual const uint32_t* pixels() const = 0;
    virtual int width() const = 0;
    virtual int height() const = 0;
};

std::unique_ptr<px_mac_offscreen> px_create_metal_offscreen(double width,
                                                            double height,
                                                            double dpi_scale);
std::unique_ptr<px_mac_offscreen> px_create_gl_offscreen(double width,
                                                         double height,
                                                         double dpi_scale);
