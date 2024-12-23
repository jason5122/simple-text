#pragma once

#include "app/app.h"
#include "experiments/resizable_widget/resizable_widget_window.h"

#include <memory>
#include <vector>

class ResizableWidgetApp : public app::App {
public:
    void createWindow();
    void destroyWindow(int wid);

    void onLaunch() override;

private:
    friend class ResizableWidgetWindow;

    std::vector<std::unique_ptr<ResizableWidgetWindow>> editor_windows;
};
