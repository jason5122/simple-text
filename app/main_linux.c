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

static const size_t default_width = 1728;
static const size_t default_height = 1041;

struct client {
    struct wl_display* display;
    struct wl_compositor* compositor;
    struct wl_seat* seat;
    struct wl_keyboard* keyboard;
    EGLDisplay egl_display;
    EGLContext egl_context;
};

struct window {
    struct client* client;
    struct wl_surface* surface;
    struct libdecor_frame* frame;
    struct wl_egl_window* egl_window;
    EGLSurface egl_surface;
    int content_width;
    int content_height;
    int floating_width;
    int floating_height;
    bool open;
    bool configured;
};

static void frame_configure(struct libdecor_frame* frame,
                            struct libdecor_configuration* configuration, void* user_data) {
    struct window* window = user_data;
    struct libdecor_state* state;
    int width, height;

    if (!libdecor_configuration_get_content_size(configuration, frame, &width, &height)) {
        width = window->floating_width;
        height = window->floating_height;
    }

    window->content_width = width;
    window->content_height = height;

    wl_egl_window_resize(window->egl_window, window->content_width, window->content_height, 0, 0);

    state = libdecor_state_new(width, height);
    libdecor_frame_commit(frame, state, configuration);
    libdecor_state_free(state);

    /* store floating dimensions */
    if (libdecor_frame_is_floating(window->frame)) {
        window->floating_width = width;
        window->floating_height = height;
    }

    window->configured = true;
}

static void frame_close(struct libdecor_frame* frame, void* user_data) {
    struct window* window = user_data;

    window->open = false;
}

static void frame_commit(struct libdecor_frame* frame, void* user_data) {
    struct window* window = user_data;

    eglSwapBuffers(window->client->display, window->egl_surface);
}

static struct libdecor_frame_interface frame_interface = {
    frame_configure,
    frame_close,
    frame_commit,
};

static void libdecor_error(struct libdecor* context, enum libdecor_error error,
                           const char* message) {
    fprintf(stderr, "Caught error (%d): %s\n", error, message);
    exit(EXIT_FAILURE);
}

static struct libdecor_interface libdecor_interface = {
    libdecor_error,
};

static void keyboard_handle_keymap(void* data, struct wl_keyboard* keyboard, uint32_t format,
                                   int fd, uint32_t size) {}

static void keyboard_handle_enter(void* data, struct wl_keyboard* keyboard, uint32_t serial,
                                  struct wl_surface* surface, struct wl_array* keys) {
    fprintf(stderr, "Keyboard gained focus\n");
}

static void keyboard_handle_leave(void* data, struct wl_keyboard* keyboard, uint32_t serial,
                                  struct wl_surface* surface) {
    fprintf(stderr, "Keyboard lost focus\n");
}

static void keyboard_handle_key(void* data, struct wl_keyboard* keyboard, uint32_t serial,
                                uint32_t time, uint32_t key, uint32_t state) {
    struct window* window = data;

    fprintf(stderr, "Key is %d state is %d\n", key, state);
    if (key == 1) {
        window->open = false;
    }
}

static void keyboard_handle_modifiers(void* data, struct wl_keyboard* keyboard, uint32_t serial,
                                      uint32_t mods_depressed, uint32_t mods_latched,
                                      uint32_t mods_locked, uint32_t group) {
    fprintf(stderr, "Modifiers depressed %d, latched %d, locked %d, group %d\n", mods_depressed,
            mods_latched, mods_locked, group);
}

static const struct wl_keyboard_listener keyboard_listener = {
    keyboard_handle_keymap, keyboard_handle_enter,     keyboard_handle_leave,
    keyboard_handle_key,    keyboard_handle_modifiers,
};

static void seat_handle_capabilities(void* data, struct wl_seat* seat,
                                     enum wl_seat_capability caps) {
    if (caps & WL_SEAT_CAPABILITY_POINTER) {
        fprintf(stderr, "Display has a pointer\n");
    }
    if (caps & WL_SEAT_CAPABILITY_KEYBOARD) {
        fprintf(stderr, "Display has a keyboard\n");
    }
    if (caps & WL_SEAT_CAPABILITY_TOUCH) {
        fprintf(stderr, "Display has a touch screen\n");
    }

    struct window* window = data;
    struct client* client = window->client;

    if (caps & WL_SEAT_CAPABILITY_KEYBOARD) {
        client->keyboard = wl_seat_get_keyboard(seat);
        wl_keyboard_add_listener(client->keyboard, &keyboard_listener, window);
    } else if (!(caps & WL_SEAT_CAPABILITY_KEYBOARD)) {
        wl_keyboard_destroy(client->keyboard);
        client->keyboard = NULL;
    }
}

static const struct wl_seat_listener seat_listener = {
    seat_handle_capabilities,
};

static void registry_global(void* data, struct wl_registry* wl_registry, uint32_t name,
                            const char* interface, uint32_t version) {
    struct window* window = data;
    struct client* client = window->client;

    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        client->compositor = wl_registry_bind(wl_registry, name, &wl_compositor_interface, 1);
    }
    if (strcmp(interface, wl_seat_interface.name) == 0) {
        client->seat = wl_registry_bind(wl_registry, name, &wl_seat_interface, 1);
        wl_seat_add_listener(client->seat, &seat_listener, window);
    }
}

static void registry_global_remove(void* data, struct wl_registry* wl_registry, uint32_t name) {}

static const struct wl_registry_listener registry_listener = {registry_global,
                                                              registry_global_remove};

static bool setup(struct window* window) {
    static const EGLint config_attribs[] = {
        EGL_SURFACE_TYPE,    EGL_WINDOW_BIT, EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT, EGL_NONE};

    EGLint major, minor;
    EGLint n;
    EGLConfig config;

    window->client->egl_display = eglGetDisplay((EGLNativeDisplayType)window->client->display);

    if (eglInitialize(window->client->egl_display, &major, &minor) == EGL_FALSE) {
        fprintf(stderr, "Cannot initialise EGL!\n");
        return false;
    }

    if (eglBindAPI(EGL_OPENGL_API) == EGL_FALSE) {
        fprintf(stderr, "Cannot bind EGL API!\n");
        return false;
    }

    if (eglChooseConfig(window->client->egl_display, config_attribs, &config, 1, &n) ==
        EGL_FALSE) {
        fprintf(stderr, "No matching EGL configurations!\n");
        return false;
    }

    window->client->egl_context =
        eglCreateContext(window->client->egl_display, config, EGL_NO_CONTEXT, NULL);

    if (window->client->egl_context == EGL_NO_CONTEXT) {
        fprintf(stderr, "No EGL context!\n");
        return false;
    }

    window->surface = wl_compositor_create_surface(window->client->compositor);

    window->egl_window = wl_egl_window_create(window->surface, default_width, default_height);

    window->egl_surface = eglCreateWindowSurface(window->client->egl_display, config,
                                                 (EGLNativeWindowType)window->egl_window, NULL);

    eglMakeCurrent(window->client->egl_display, window->egl_surface, window->egl_surface,
                   window->client->egl_context);

    return true;
}

static void cleanup(struct window* window) {
    if (window->client->egl_display) {
        eglMakeCurrent(window->client->egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE,
                       EGL_NO_CONTEXT);
    }
    if (window->egl_surface) {
        eglDestroySurface(window->client->egl_display, window->egl_surface);
    }
    if (window->egl_window) {
        wl_egl_window_destroy(window->egl_window);
    }
    if (window->surface) {
        wl_surface_destroy(window->surface);
    }
    if (window->client->egl_context) {
        eglDestroyContext(window->client->egl_display, window->client->egl_context);
    }
    if (window->client->egl_display) {
        eglTerminate(window->client->egl_display);
    }
}

static float hue_to_channel(const float* const hue, const int n) {
    /* convert hue to rgb channels with saturation and value equal to 1
     * https://en.wikipedia.org/wiki/HSL_and_HSV#HSV_to_RGB_alternative
     */
    const float k = fmod(n + ((*hue) * 3 / M_PI), 6);
    return 1 - MAX(0, MIN(MIN(k, 4 - k), 1));
}

static void hue_to_rgb(const float* const hue, float (*rgb)[3]) {
    (*rgb)[0] = hue_to_channel(hue, 5);
    (*rgb)[1] = hue_to_channel(hue, 3);
    (*rgb)[2] = hue_to_channel(hue, 1);
}

static void draw(struct window* window) {
    struct timespec tv;
    double time;

    /* change of colour hue (HSV space) in rad/sec */
    static const float hue_change = (2 * M_PI) / 10;
    float hue;
    float rgb[3] = {0, 0, 0};

    clock_gettime(CLOCK_REALTIME, &tv);
    time = tv.tv_sec + tv.tv_nsec * 1e-9;

    hue = fmod(time * hue_change, 2 * M_PI);

    hue_to_rgb(&hue, &rgb);

    glViewport(0, 0, window->content_width, window->content_height);
    glClearColor(rgb[0], rgb[1], rgb[2], 1);
    glClear(GL_COLOR_BUFFER_BIT);

    float vertices[] = {-0.5, -0.5, 0.0, 0.5, 0.5, -0.5};
    glVertexPointer(2, GL_FLOAT, 0, vertices);
    glEnableClientState(GL_VERTEX_ARRAY);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    eglSwapBuffers(window->client->egl_display, window->egl_surface);
}

int SimpleTextMain() {
    struct wl_registry* wl_registry;
    struct libdecor* context = NULL;
    struct window* window;
    struct client* client;
    int ret = EXIT_SUCCESS;

    client = calloc(1, sizeof(struct client));

    client->display = wl_display_connect(NULL);
    if (!client->display) {
        fprintf(stderr, "No Wayland connection\n");
        free(client);
        return EXIT_FAILURE;
    }

    window = calloc(1, sizeof(struct window));
    window->client = client;
    window->open = true;
    window->configured = false;
    window->floating_width = default_width;
    window->floating_height = default_height;

    wl_registry = wl_display_get_registry(client->display);
    wl_registry_add_listener(wl_registry, &registry_listener, window);
    wl_display_roundtrip(client->display);

    if (!setup(window)) {
        goto out;
    }

    context = libdecor_new(client->display, &libdecor_interface);
    window->frame = libdecor_decorate(context, window->surface, &frame_interface, window);
    libdecor_frame_set_app_id(window->frame, "simple-text");
    libdecor_frame_set_title(window->frame, "Simple Text");
    libdecor_frame_map(window->frame);

    wl_display_roundtrip(client->display);
    wl_display_roundtrip(client->display);

    /* wait for the first configure event */
    while (!window->configured) {
        if (libdecor_dispatch(context, 0) < 0) {
            ret = EXIT_FAILURE;
            goto out;
        }
    }

    printf("%s\n", glGetString(GL_VERSION));

    while (window->open) {
        if (libdecor_dispatch(context, 0) < 0) {
            ret = EXIT_FAILURE;
            goto out;
        }
        draw(window);
    }

out:
    if (context) {
        libdecor_unref(context);
    }
    cleanup(window);
    free(window);
    free(client);

    return ret;
}
