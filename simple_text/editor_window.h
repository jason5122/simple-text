#pragma once

#include "config/color_scheme.h"
#include "gui/window.h"
#include "renderer/renderer.h"
#include "simple_text/editor_tab.h"

class SimpleText;

class EditorWindow : public gui::Window {
public:
    int wid;

    NOT_COPYABLE(EditorWindow)
    NOT_MOVABLE(EditorWindow)
    EditorWindow(SimpleText& parent, int width, int height, int wid);
    ~EditorWindow() override;
    void createTab(fs::path file_path);
    void reloadColorScheme();
    void selectTabIndex(int index);
    void selectPreviousTab();
    void selectNextTab();
    void selectLastTab();
    void closeCurrentTab();
    void toggleSideBar();

    void onOpenGLActivate(int width, int height) override;
    void onDraw() override;
    void onResize(int width, int height) override;
    void onScroll(int dx, int dy) override;
    void onLeftMouseDown(int mouse_x, int mouse_y, gui::ModifierKey modifiers) override;
    void onLeftMouseDrag(int mouse_x, int mouse_y, gui::ModifierKey modifiers) override;
    void onKeyDown(gui::Key key, gui::ModifierKey modifiers) override;
    void onClose() override;
    void onDarkModeToggle() override;

private:
    SimpleText& parent;

    int tab_index = 0;  // Use int instead of size_t to prevent wrap around when less than 0.
    std::vector<std::unique_ptr<EditorTab>> tabs;

    config::ColorScheme color_scheme;

// TODO: Figure out OpenGL context reuse on Linux.
#if IS_MAC || IS_WIN
    renderer::Renderer& renderer;
#elif IS_LINUX
    renderer::Renderer renderer;
#endif
};
