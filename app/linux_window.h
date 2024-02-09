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
