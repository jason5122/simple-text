#pragma once

#include "third_party/libdecor/src/libdecor.h"
#include "third_party/libdecor/src/utils.h"
#include "ui/linux/wayland_client.h"
#include <EGL/egl.h>
#include <fstream>
#include <glad/glad.h>
#include <wayland-client.h>
#include <wayland-egl.h>

inline const char* ReadFileCpp(const char* file_name) {
    std::ifstream in(file_name);
    static std::string contents((std::istreambuf_iterator<char>(in)),
                                std::istreambuf_iterator<char>());
    return contents.c_str();
}

class WaylandWindow {
public:
    WaylandWindow(WaylandClient client);
    bool setup();
    void draw();
    ~WaylandWindow();

    WaylandClient client;

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

    GLuint shader_program;

    void linkShaders();
};
