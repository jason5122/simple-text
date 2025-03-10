#pragma once

#include "gui/platform/action.h"
#include "gui/platform/key.h"
#include "gui/platform/window_widget.h"
#include "util/non_copyable.h"

#include <memory>

namespace gui {

class App {
public:
    App();
    virtual ~App();
    void run();
    void quit();
    std::string getClipboardString();
    void setClipboardString(const std::string& str8);

    virtual void onLaunch() {}
    virtual void onQuit() {}
    virtual void onAppAction(AppAction action) {}

    // TODO: Find a way to make this private! We currently need this for GTK's callbacks.
    class impl;
    std::unique_ptr<impl> pimpl;

private:
    friend class WindowWidget;

    // class impl;
    // std::unique_ptr<impl> pimpl;
};

}  // namespace gui
