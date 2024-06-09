#include "build/buildflag.h"
#include "editor_window.h"
#include "renderer/opengl_functions.h"
#include "simple_text/simple_text.h"
#include <cctype>

#include <format>
#include <iostream>

// TODO: Temporary; remove this.
#include "opengl/functions_gl.h"
#include <dlfcn.h>
namespace {
const char* kDefaultOpenGLDylibName =
    "/System/Library/Frameworks/OpenGL.framework/Libraries/libGL.dylib";
}

EditorWindow::EditorWindow(SimpleText& parent, int width, int height, int wid)
    : Window(parent), parent(parent), wid(wid), color_scheme(isDarkMode()) {}

void EditorWindow::onOpenGLActivate(int width, int height) {
    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);

    glClearColor(0.5f, 0.0f, 0.0f, 1.0f);

    GLuint tex_id;
    glGenTextures(1, &tex_id);
    std::cerr << std::format("tex_id = {}", tex_id) << '\n';
}

void EditorWindow::onDraw() {
    void* handle = dlopen(kDefaultOpenGLDylibName, RTLD_NOW);
    if (!handle) {
        std::cerr << "Could not open the OpenGL Framework.\n";
    }

    std::unique_ptr<opengl::FunctionsGL> functionsGL(new opengl::FunctionsGL(handle));
    // std::unique_ptr<FunctionsGL> functionsGL(new FunctionsGLCGL(handle));
    functionsGL->initialize();

    functionsGL->clear(GL_COLOR_BUFFER_BIT);
    // glClear(GL_COLOR_BUFFER_BIT);
}

void EditorWindow::onResize(int width, int height) {
    redraw();
}

void EditorWindow::onClose() {
    parent.destroyWindow(wid);
}
