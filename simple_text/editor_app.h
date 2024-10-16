#pragma once

#include "app/app.h"
#include "simple_text/editor_window.h"
#include <vector>

class EditorApp : public app::App {
public:
    void createWindow();
    void destroyWindow(int wid);

    void onLaunch() override;
    void onQuit() override;
    void onAppAction(app::AppAction action) override;

private:
    friend class EditorWindow;

    std::vector<std::unique_ptr<EditorWindow>> editor_windows;
};
