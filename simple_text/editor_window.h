#pragma once

#include "build/buildflag.h"
#include "config/color_scheme.h"
#include "gui/window.h"
#include "renderer/renderer.h"

class SimpleText;

class EditorWindow : public gui::Window {
public:
    int wid;

    EditorWindow(SimpleText& parent, int width, int height, int wid);

    void onOpenGLActivate(int width, int height) override;
    void onDraw() override;
    void onResize(int width, int height) override;
    void onClose() override;

private:
    SimpleText& parent;

#if IS_MAC || IS_WINDOWS
    renderer::Renderer& renderer;
#elif IS_LINUX
    renderer::Renderer renderer;
#endif

    config::ColorScheme color_scheme;

    void updateWindowTitle();
};
