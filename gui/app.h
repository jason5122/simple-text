#pragma once

#include "gui/gui_action.h"
#include "gui/key.h"
#include "gui/modifier_key.h"
#include "gui/window.h"
#include "util/not_copyable_or_movable.h"
#include <memory>

namespace gui {

class App {
public:
    NOT_COPYABLE(App)
    NOT_MOVABLE(App)
    App();
    virtual ~App();
    void run();
    void quit();

    virtual void onLaunch() {}
    virtual void onQuit() {}
    virtual void onGuiAction(GuiAction action) {}

private:
    friend class Window;

    class impl;
    std::unique_ptr<impl> pimpl;
};

}
