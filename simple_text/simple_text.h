#pragma once

#include "base/filesystem/file_watcher.h"
#include "config/key_bindings.h"
#include "font/rasterizer.h"
#include "gui/app.h"
#include "renderer/renderer.h"
#include "simple_text/editor_window.h"
#include "util/non_copyable.h"
#include <vector>

class SimpleText : public gui::App, public FileWatcherCallback {
public:
    SimpleText();
    ~SimpleText() override;
    void createWindow();
    void destroyWindow(int wid);
    void createNWindows(int n);
    void destroyAllWindows();

    void onLaunch() override;
    void onQuit() override;
    void onGuiAction(gui::GuiAction action) override;
    void onFileEvent() override;

private:
    friend class EditorWindow;

    std::vector<std::unique_ptr<EditorWindow>> editor_windows;

#if IS_MAC || IS_WIN
    renderer::Renderer renderer;
#endif
    config::KeyBindings key_bindings;
    FileWatcher file_watcher;

    font::FontRasterizer main_font_rasterizer;
    font::FontRasterizer ui_font_rasterizer;
};
