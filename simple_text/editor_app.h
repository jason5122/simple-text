#pragma once

#include "app/app.h"
#include "opengl/functions_gl.h"
#include "renderer/renderer.h"
#include "simple_text/editor_window.h"
#include <vector>

class EditorApp : public app::App {
public:
    EditorApp();

    void createWindow();
    void destroyWindow(int wid);

    void onLaunch() override;
    void onQuit() override;

private:
    friend class EditorWindow;

    std::vector<std::unique_ptr<EditorWindow>> editor_windows;

    std::unique_ptr<opengl::FunctionsGL> gl;
    std::unique_ptr<renderer::Renderer> renderer;
};
