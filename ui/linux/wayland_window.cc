#include "wayland_window.h"
#include <iostream>
#include <stdio.h>
#include <time.h>

WaylandWindow::WaylandWindow(WaylandClient client)
    : client(client), floating_width(DEFAULT_WIDTH), floating_height(DEFAULT_HEIGHT), open(true),
      configured(false) {}

bool WaylandWindow::setup() {
    static const EGLint config_attribs[] = {
        EGL_SURFACE_TYPE,    EGL_WINDOW_BIT, EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT, EGL_NONE};

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

    std::cerr << glGetString(GL_VERSION) << '\n';

    this->linkShaders();

    return true;
}

static float hue_to_channel(const float* const hue, const int n) {
    // Convert hue to rgb channels with saturation and value equal to 1.
    // https://en.wikipedia.org/wiki/HSL_and_HSV#HSV_to_RGB_alternative
    const float k = fmod(n + ((*hue) * 3 / M_PI), 6);
    return 1 - MAX(0, MIN(MIN(k, 4 - k), 1));
}

static void hue_to_rgb(const float* const hue, float (*rgb)[3]) {
    (*rgb)[0] = hue_to_channel(hue, 5);
    (*rgb)[1] = hue_to_channel(hue, 3);
    (*rgb)[2] = hue_to_channel(hue, 1);
}

void WaylandWindow::draw() {
    timespec tv;
    double time;

    // Change of color hue (HSV space) in rad/sec.
    static const float hue_change = (2 * M_PI) / 10;
    float hue;
    float rgb[3] = {0, 0, 0};

    clock_gettime(CLOCK_REALTIME, &tv);
    time = tv.tv_sec + tv.tv_nsec * 1e-9;

    hue = fmod(time * hue_change, 2 * M_PI);

    hue_to_rgb(&hue, &rgb);

    glViewport(0, 0, content_width, content_height);
    glClearColor(rgb[0], rgb[1], rgb[2], 1);
    glClear(GL_COLOR_BUFFER_BIT);

    float vertices[] = {-0.5, -0.5, 0.0, 0.5, 0.5, -0.5};
    glVertexPointer(2, GL_FLOAT, 0, vertices);
    glEnableClientState(GL_VERTEX_ARRAY);
    glDrawArrays(GL_TRIANGLES, 0, 3);

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

void WaylandWindow::linkShaders() {
    // GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    // GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    const GLchar* vert_source = ReadFileCpp("shaders/triangle_vert.glsl");
    const GLchar* frag_source = ReadFileCpp("shaders/triangle_frag.glsl");

    // glShaderSource(vertex_shader, 1, &vert_source, nullptr);
    // glShaderSource(fragment_shader, 1, &frag_source, nullptr);
    // glCompileShader(vertex_shader);
    // glCompileShader(fragment_shader);

    // shader_program = glCreateProgram();
    // glAttachShader(shader_program, vertex_shader);
    // glAttachShader(shader_program, fragment_shader);
    // glLinkProgram(shader_program);

    // glDeleteShader(vertex_shader);
    // glDeleteShader(fragment_shader);
}
