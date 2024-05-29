#pragma once

#include "base/filesystem/file_watcher.h"
#include "config/key_bindings.h"
#include "font/rasterizer.h"
#include "gui/app.h"
#include "renderer/renderer.h"
#include "simple_text/editor_window.h"
#include "util/not_copyable_or_movable.h"
#include <vector>

class SimpleText : public App, public FileWatcherCallback {
public:
    // TODO: Make these private or move them to another class.
    font::FontRasterizer main_font_rasterizer;
    font::FontRasterizer ui_font_rasterizer;
#if IS_MAC || IS_WIN
    renderer::Renderer renderer;
#endif
    config::KeyBindings key_bindings;
    FileWatcher file_watcher;

    NOT_COPYABLE(SimpleText)
    NOT_MOVABLE(SimpleText)
    SimpleText();
    ~SimpleText() override;
    void createWindow();
    void destroyWindow(int wid);
    void createNWindows(int n);
    void destroyAllWindows();

    void onLaunch() override;
    void onQuit() override;
    void onFileEvent() override;

private:
    std::vector<std::unique_ptr<EditorWindow>> editor_windows;
};
