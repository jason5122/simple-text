#include "linux_window.h"
#include <iostream>

static void registry_global(void* data, struct wl_registry* wl_registry, uint32_t name,
                            const char* interface, uint32_t version) {
    Window* window = static_cast<Window*>(data);
    struct client* client = window->client;

    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        client->compositor = static_cast<struct wl_compositor*>(
            wl_registry_bind(wl_registry, name, &wl_compositor_interface, 1));
    }
    // if (strcmp(interface, wl_seat_interface.name) == 0) {
    //     client->seat = static_cast<struct wl_seat*>(
    //         wl_registry_bind(wl_registry, name, &wl_seat_interface, 1));
    //     wl_seat_add_listener(client->seat, &seat_listener, window);
    // }
}

static void registry_global_remove(void* data, struct wl_registry* wl_registry, uint32_t name) {}

static const struct wl_registry_listener registry_listener = {registry_global,
                                                              registry_global_remove};

Window::Window(struct libdecor* context)
    : floating_width(DEFAULT_WIDTH), floating_height(DEFAULT_HEIGHT) {
    client = static_cast<struct client*>(calloc(1, sizeof(struct client)));

    client->display = wl_display_connect(NULL);
    if (!client->display) {
        fprintf(stderr, "No Wayland connection\n");
        free(client);
        // return EXIT_FAILURE;
    }

    wl_registry = wl_display_get_registry(client->display);
    wl_registry_add_listener(wl_registry, &registry_listener, this);
    wl_display_roundtrip(client->display);
}

bool Window::setup() {
    static const EGLint config_attribs[] = {
        EGL_SURFACE_TYPE,    EGL_WINDOW_BIT, EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT, EGL_NONE};

    EGLint major, minor;
    EGLint n;
    EGLConfig config;

    client->egl_display = eglGetDisplay((EGLNativeDisplayType)client->display);

    if (eglInitialize(client->egl_display, &major, &minor) == EGL_FALSE) {
        fprintf(stderr, "Cannot initialise EGL!\n");
        return false;
    }

    if (eglBindAPI(EGL_OPENGL_API) == EGL_FALSE) {
        fprintf(stderr, "Cannot bind EGL API!\n");
        return false;
    }

    if (eglChooseConfig(client->egl_display, config_attribs, &config, 1, &n) == EGL_FALSE) {
        fprintf(stderr, "No matching EGL configurations!\n");
        return false;
    }

    client->egl_context = eglCreateContext(client->egl_display, config, EGL_NO_CONTEXT, NULL);

    if (client->egl_context == EGL_NO_CONTEXT) {
        fprintf(stderr, "No EGL context!\n");
        return false;
    }

    std::cout << client->compositor << '\n';

    surface = wl_compositor_create_surface(client->compositor);

    egl_window = wl_egl_window_create(surface, DEFAULT_WIDTH, DEFAULT_HEIGHT);

    egl_surface =
        eglCreateWindowSurface(client->egl_display, config, (EGLNativeWindowType)egl_window, NULL);

    // eglMakeCurrent(client->egl_display, egl_surface, egl_surface, client->egl_context);

    return true;
}
