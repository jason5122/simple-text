#pragma once

#include "config/key_bindings.h"
#include "gui/app.h"
#include "util/not_copyable_or_movable.h"
#include <vector>

#include "config/color_scheme.h"
#include "font/rasterizer.h"
#include "renderer/renderer.h"
#include "simple_text/editor_tab.h"

#include "base/filesystem/file_watcher.h"

class SimpleText : public App, public FileWatcherCallback {
public:
    class EditorWindow : public App::Window {
    public:
        int wid;

        NOT_COPYABLE(EditorWindow)
        NOT_MOVABLE(EditorWindow)
        EditorWindow(SimpleText& parent, int width, int height, int wid);
        ~EditorWindow() override;
        void createTab(fs::path file_path);
        void reloadColorScheme();

        void onOpenGLActivate(int width, int height) override;
        void onDraw() override;
        void onResize(int width, int height) override;
        void onScroll(int dx, int dy) override;
        void onLeftMouseDown(int mouse_x, int mouse_y, app::ModifierKey modifiers) override;
        void onLeftMouseDrag(int mouse_x, int mouse_y, app::ModifierKey modifiers) override;
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

        static constexpr int kLineNumberOffset = 120;

        int tab_index = 0;  // Use int instead of size_t to prevent wrap around when less than 0.
        std::vector<std::unique_ptr<EditorTab>> tabs;

        config::ColorScheme color_scheme;

        font::FontRasterizer& main_font_rasterizer;
        font::FontRasterizer& ui_font_rasterizer;

// TODO: Figure out OpenGL context reuse on Linux.
#if IS_MAC || IS_WIN
        renderer::Renderer& renderer;
#elif IS_LINUX
        renderer::Renderer renderer;
#endif
    };

    NOT_COPYABLE(SimpleText)
    NOT_MOVABLE(SimpleText)
    SimpleText();
    ~SimpleText() override;
    void createWindow();
    void destroyWindow(int wid);
    void createNWindows(int n);
    void destroyAllWindows();

    void onLaunch() override;
    void onFileEvent() override;

private:
    std::vector<std::unique_ptr<EditorWindow>> editor_windows;

    font::FontRasterizer main_font_rasterizer;
    font::FontRasterizer ui_font_rasterizer;

#if IS_MAC || IS_WIN
    renderer::Renderer renderer;
#endif

    config::KeyBindings key_bindings;
    FileWatcher file_watcher;
};
