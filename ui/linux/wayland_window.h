#pragma once

#include "third_party/libdecor/src/libdecor.h"
#include "third_party/libdecor/src/utils.h"
#include "ui/linux/wayland_client.h"
#include "ui/renderer/triangle_renderer.h"
#include <EGL/egl.h>
#include <wayland-client.h>
#include <wayland-egl.h>

class WaylandWindow {
public:
    WaylandWindow(WaylandClient client);
    bool setup();
    void draw();
    ~WaylandWindow();

    WaylandClient client;
    TriangleRenderer triangle_renderer;

    wl_surface* surface;
    libdecor_frame* frame;
    wl_egl_window* egl_window;
    EGLSurface egl_surface;
    int content_width;
    int content_height;
    int floating_width;
    int floating_height;
    bool open;
    bool configured;

private:
    static const size_t DEFAULT_WIDTH = 1728;
    static const size_t DEFAULT_HEIGHT = 1041;
};
