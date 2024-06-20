#pragma once

#include "app/app.h"
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
    std::shared_ptr<renderer::Renderer> renderer;
};
