#pragma once

#include "gui/app.h"
#include "simple_text/editor_window.h"
#include "util/non_copyable.h"
#include <vector>

class SimpleText : public gui::App {
public:
    SimpleText();
    ~SimpleText() override;
    void createWindow();
    void destroyWindow(int wid);

    void onLaunch() override;
    void onQuit() override;

private:
    friend class EditorWindow;

    std::vector<std::unique_ptr<EditorWindow>> editor_windows;
};
