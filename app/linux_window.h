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

class Client {
public:
    wl_display* display;
    wl_compositor* compositor;
    wl_seat* seat;
    wl_keyboard* keyboard;
    EGLDisplay egl_display;
    EGLContext egl_context;
};

class Window {
public:
    Window() = default;
    bool setup();
    void draw();
    ~Window();

    Client* client;

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
};
