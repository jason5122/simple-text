#include "third_party/libdecor/src/libdecor.h"
#include "third_party/libdecor/src/utils.h"
#include <EGL/egl.h>
#include <GL/gl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wayland-client.h>
#include <wayland-egl.h>

static const size_t DEFAULT_WIDTH = 1728;
static const size_t DEFAULT_HEIGHT = 1041;

struct client {
    struct wl_display* display;
    struct wl_compositor* compositor;
    struct wl_seat* seat;
    struct wl_keyboard* keyboard;
    EGLDisplay egl_display;
    EGLContext egl_context;
};

class Window {
public:
    struct client* client;

    Window(struct libdecor* context);
    bool setup();

private:
    struct wl_registry* wl_registry;

    struct wl_surface* surface;
    struct libdecor_frame* frame;
    struct wl_egl_window* egl_window;
    EGLSurface egl_surface;
    int content_width;
    int content_height;
    int floating_width;
    int floating_height;
    bool open = true;
    bool configured = false;
};
