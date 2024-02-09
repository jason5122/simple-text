#include "wayland_window.h"
#include <iostream>
#include <stdio.h>

WaylandWindow::WaylandWindow(WaylandClient client)
    : client(client), floating_width(DEFAULT_WIDTH), floating_height(DEFAULT_HEIGHT), open(true),
      configured(false) {}

bool WaylandWindow::setup() {
    static const EGLint config_attribs[] = {
        EGL_SURFACE_TYPE,    EGL_WINDOW_BIT, EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT, EGL_NONE,
    };

    EGLint major, minor;
    EGLint n;
    EGLConfig config;

    client.egl_display = eglGetDisplay((EGLNativeDisplayType)client.display);

    if (eglInitialize(client.egl_display, &major, &minor) == EGL_FALSE) {
        fprintf(stderr, "Cannot initialise EGL!\n");
        return false;
    }

    if (eglBindAPI(EGL_OPENGL_API) == EGL_FALSE) {
        fprintf(stderr, "Cannot bind EGL API!\n");
        return false;
    }

    if (eglChooseConfig(client.egl_display, config_attribs, &config, 1, &n) == EGL_FALSE) {
        fprintf(stderr, "No matching EGL configurations!\n");
        return false;
    }

    client.egl_context = eglCreateContext(client.egl_display, config, EGL_NO_CONTEXT, NULL);

    if (client.egl_context == EGL_NO_CONTEXT) {
        fprintf(stderr, "No EGL context!\n");
        return false;
    }

    surface = wl_compositor_create_surface(client.compositor);

    egl_window = wl_egl_window_create(surface, DEFAULT_WIDTH, DEFAULT_HEIGHT);

    egl_surface =
        eglCreateWindowSurface(client.egl_display, config, (EGLNativeWindowType)egl_window, NULL);

    eglMakeCurrent(client.egl_display, egl_surface, egl_surface, client.egl_context);

    if (!gladLoadGL()) {
        std::cerr << "Failed to initialize GLAD\n";
        return false;
    }

    std::cerr << glGetString(GL_VERSION) << '\n';
    triangle_renderer.setup();

    return true;
}

void WaylandWindow::draw() {
    glViewport(0, 0, content_width, content_height);
    triangle_renderer.draw();

    eglSwapBuffers(client.egl_display, egl_surface);
}

WaylandWindow::~WaylandWindow() {
    if (client.egl_display) {
        eglMakeCurrent(client.egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    }
    if (egl_surface) {
        eglDestroySurface(client.egl_display, egl_surface);
    }
    if (egl_window) {
        wl_egl_window_destroy(egl_window);
    }
    if (surface) {
        wl_surface_destroy(surface);
    }
    if (client.egl_context) {
        eglDestroyContext(client.egl_display, client.egl_context);
    }
    if (client.egl_display) {
        eglTerminate(client.egl_display);
    }
}
