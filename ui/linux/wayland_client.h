#pragma once

#include <EGL/egl.h>
#include <wayland-client.h>
#include <wayland-egl.h>

class WaylandClient {
public:
    WaylandClient() = default;
    bool connectToDisplay();

    wl_display* display;
    wl_compositor* compositor;
    wl_seat* seat;
    wl_keyboard* keyboard;
    EGLDisplay egl_display;
    EGLContext egl_context;
};
