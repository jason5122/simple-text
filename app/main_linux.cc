#include "app/linux_window.h"
#include "third_party/libdecor/src/libdecor.h"
#include "third_party/libdecor/src/utils.h"
#include <EGL/egl.h>
#include <GL/gl.h>
#include <iostream>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wayland-client.h>
#include <wayland-egl.h>

static void frame_configure(libdecor_frame* frame, libdecor_configuration* configuration,
                            void* user_data) {
    Window* window = static_cast<class Window*>(user_data);
    libdecor_state* state;
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

static void frame_close(libdecor_frame* frame, void* user_data) {
    Window* window = static_cast<class Window*>(user_data);

    window->open = false;
}

static void frame_commit(libdecor_frame* frame, void* user_data) {
    Window* window = static_cast<class Window*>(user_data);

    eglSwapBuffers(window->client->display, window->egl_surface);
}

static libdecor_frame_interface frame_interface = {
    frame_configure,
    frame_close,
    frame_commit,
};

static void libdecor_error(libdecor* context, enum libdecor_error error, const char* message) {
    fprintf(stderr, "Caught error (%d): %s\n", error, message);
    exit(EXIT_FAILURE);
}

static libdecor_interface libdecor_interface = {
    libdecor_error,
};

static void keyboard_handle_keymap(void* data, wl_keyboard* keyboard, uint32_t format, int fd,
                                   uint32_t size) {}

static void keyboard_handle_enter(void* data, wl_keyboard* keyboard, uint32_t serial,
                                  wl_surface* surface, wl_array* keys) {
    fprintf(stderr, "Keyboard gained focus\n");
}

static void keyboard_handle_leave(void* data, wl_keyboard* keyboard, uint32_t serial,
                                  wl_surface* surface) {
    fprintf(stderr, "Keyboard lost focus\n");
}

static void keyboard_handle_key(void* data, wl_keyboard* keyboard, uint32_t serial, uint32_t time,
                                uint32_t key, uint32_t state) {
    Window* window = static_cast<class Window*>(data);

    fprintf(stderr, "Key is %d state is %d\n", key, state);
    if (key == 1) {
        window->open = false;
    }
}

static void keyboard_handle_modifiers(void* data, wl_keyboard* keyboard, uint32_t serial,
                                      uint32_t mods_depressed, uint32_t mods_latched,
                                      uint32_t mods_locked, uint32_t group) {
    fprintf(stderr, "Modifiers depressed %d, latched %d, locked %d, group %d\n", mods_depressed,
            mods_latched, mods_locked, group);
}

static const wl_keyboard_listener keyboard_listener = {
    keyboard_handle_keymap, keyboard_handle_enter,     keyboard_handle_leave,
    keyboard_handle_key,    keyboard_handle_modifiers,
};

static void seat_handle_capabilities(void* data, wl_seat* seat, uint32_t caps) {
    if (caps & WL_SEAT_CAPABILITY_POINTER) {
        fprintf(stderr, "Display has a pointer\n");
    }
    if (caps & WL_SEAT_CAPABILITY_KEYBOARD) {
        fprintf(stderr, "Display has a keyboard\n");
    }
    if (caps & WL_SEAT_CAPABILITY_TOUCH) {
        fprintf(stderr, "Display has a touch screen\n");
    }

    Window* window = static_cast<class Window*>(data);
    Client* client = window->client;

    if (caps & WL_SEAT_CAPABILITY_KEYBOARD) {
        client->keyboard = wl_seat_get_keyboard(seat);
        wl_keyboard_add_listener(client->keyboard, &keyboard_listener, window);
    } else if (!(caps & WL_SEAT_CAPABILITY_KEYBOARD)) {
        wl_keyboard_destroy(client->keyboard);
        client->keyboard = NULL;
    }
}

static const wl_seat_listener seat_listener = {
    seat_handle_capabilities,
};

static void registry_global(void* data, wl_registry* wl_registry, uint32_t name,
                            const char* interface, uint32_t version) {
    Window* window = static_cast<class Window*>(data);
    Client* client = window->client;

    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        client->compositor = static_cast<class wl_compositor*>(
            wl_registry_bind(wl_registry, name, &wl_compositor_interface, 1));
    }
    if (strcmp(interface, wl_seat_interface.name) == 0) {
        client->seat = static_cast<class wl_seat*>(
            wl_registry_bind(wl_registry, name, &wl_seat_interface, 1));
        wl_seat_add_listener(client->seat, &seat_listener, window);
    }
}

static void registry_global_remove(void* data, wl_registry* wl_registry, uint32_t name) {}

static const wl_registry_listener registry_listener = {registry_global, registry_global_remove};

int SimpleTextMain() {
    wl_registry* wl_registry;
    libdecor* context = NULL;
    int ret = EXIT_SUCCESS;

    Client* client = new Client();

    client->display = wl_display_connect(NULL);
    if (!client->display) {
        fprintf(stderr, "No Wayland connection\n");
        free(client);
        return EXIT_FAILURE;
    }

    Window* window = new Window();
    window->client = client;

    wl_registry = wl_display_get_registry(client->display);
    wl_registry_add_listener(wl_registry, &registry_listener, window);
    wl_display_roundtrip(client->display);

    if (!window->setup()) {
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

    std::cerr << glGetString(GL_VERSION) << '\n';

    while (window->open) {
        if (libdecor_dispatch(context, 0) < 0) {
            ret = EXIT_FAILURE;
            goto out;
        }
        window->draw();
    }

out:
    if (context) {
        libdecor_unref(context);
    }
    delete window;
    free(client);

    return ret;
}
