#pragma once

#include "build/buildflag.h"
#include "gui/app.h"
#include "opengl/functions_gl.h"
#include "renderer/renderer.h"
#include "simple_text/editor_window.h"
#include <vector>

class SimpleText : public gui::App {
public:
    SimpleText();

    void createWindow();
    void destroyWindow(int wid);

    void onLaunch() override;
    void onQuit() override;

private:
    friend class EditorWindow;

    std::vector<std::unique_ptr<EditorWindow>> editor_windows;

    std::unique_ptr<opengl::FunctionsGL> gl;

#if IS_MAC || IS_WINDOWS
    renderer::Renderer renderer;
#endif
};
