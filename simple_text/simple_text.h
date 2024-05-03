#pragma once

#include "gui/app.h"
#include <vector>

#include "config/color_scheme.h"
#include "font/rasterizer.h"
#include "renderer/image_renderer.h"
#include "renderer/rect_renderer.h"
#include "renderer/selection_renderer.h"
#include "renderer/text_renderer.h"
#include "simple_text/editor_tab.h"

#include "base/filesystem/file_watcher.h"

class SimpleText : public App, public FileWatcherCallback {
public:
    class EditorWindow : public App::Window {
    public:
        int wid;

        EditorWindow(SimpleText& parent, int width, int height, int wid);
        ~EditorWindow();
        void createTab(fs::path file_path);
        void createTab();
        void reloadColorScheme();

        void onOpenGLActivate(int width, int height) override;
        void onDraw() override;
        void onResize(int width, int height) override;
        void onScroll(float dx, float dy) override;
        void onLeftMouseDown(float mouse_x, float mouse_y) override;
        void onLeftMouseDrag(float mouse_x, float mouse_y) override;
        void onKeyDown(app::Key key, app::ModifierKey modifiers) override;
        void onClose() override;
        void onDarkModeToggle() override;

    private:
        SimpleText& parent;

        bool side_bar_visible = true;
        renderer::Point editor_offset{
            .x = 200 * 2,
            .y = 30 * 2,
        };

        static constexpr float kLineNumberOffset = 120;

        int tab_index = 0;
        std::vector<std::unique_ptr<EditorTab>> tabs;

        config::ColorScheme color_scheme;

// TODO: Figure out OpenGL context reuse on Linux.
#if IS_LINUX
        FontRasterizer main_font_rasterizer;
        FontRasterizer ui_font_rasterizer;
        renderer::TextRenderer text_renderer;
        renderer::RectRenderer rect_renderer;
        renderer::ImageRenderer image_renderer;
        renderer::SelectionRenderer selection_renderer;
#endif
    };

#if IS_MAC || IS_WIN
    FontRasterizer main_font_rasterizer;
    FontRasterizer ui_font_rasterizer;
    renderer::TextRenderer text_renderer;
    renderer::RectRenderer rect_renderer;
    renderer::ImageRenderer image_renderer;
    renderer::SelectionRenderer selection_renderer;
#endif

    FileWatcher file_watcher;

    SimpleText();
    ~SimpleText();
    void createWindow();
    void destroyWindow(int wid);
    void createNWindows(int n);
    void destroyAllWindows();

    void onLaunch() override;
    void onFileEvent() override;

private:
    std::vector<std::unique_ptr<EditorWindow>> editor_windows;
};
