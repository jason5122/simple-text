#include "fast_startup_app.h"

#include "base/filesystem/file_reader.h"
#include "gui/renderer/renderer.h"
#include "opengl/functions_gl.h"

#include "util/std_print.h"

// We should have an OpenGL context within this function.
// Load OpenGL function pointers and perform OpenGL setup here.
void FastStartupApp::onLaunch() {
    opengl::FunctionsGL functions_gl{};
    functions_gl.loadGlobalFunctionPointers();

    // Load images.
    auto& image_renderer = gui::Renderer::instance().getImageRenderer();
    std::string panel_close_2x = std::format("{}/icons/panel_close@2x.png", base::ResourceDir());
    std::string folder_open_2x = std::format("{}/icons/folder_open@2x.png", base::ResourceDir());
    std::string stanford_bunny = std::format("{}/icons/stanford_bunny.png", base::ResourceDir());
    std::string dice = std::format("{}/icons/dice.png", base::ResourceDir());
    std::string example_jpg = std::format("{}/icons/example.jpg", base::ResourceDir());
    std::string lcd = std::format("{}/icons/lcd.jpg", base::ResourceDir());
    kPanelClose2xIndex = image_renderer.addPng(panel_close_2x);
    kFolderOpen2xIndex = image_renderer.addPng(folder_open_2x);
    kStanfordBunny = image_renderer.addPng(stanford_bunny);
    kDice = image_renderer.addPng(dice);
    kExampleJpg = image_renderer.addJpeg(example_jpg);
    kLCD = image_renderer.addJpeg(lcd);

    createWindow();
}

void FastStartupApp::createWindow() {
    std::unique_ptr<FastStartupWindow> editor_window =
        std::make_unique<FastStartupWindow>(*this, 1200, 800, 0);

    editor_window->show();
    editor_windows.emplace_back(std::move(editor_window));
}

void FastStartupApp::destroyWindow(int wid) {
    editor_windows[wid] = nullptr;
}
