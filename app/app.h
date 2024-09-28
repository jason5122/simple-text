#pragma once

#include "app/app_action.h"
#include "app/key.h"
#include "app/modifier_key.h"
#include "app/window.h"
#include "util/non_copyable.h"
#include <memory>

namespace app {

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
    friend class Window;

    // class impl;
    // std::unique_ptr<impl> pimpl;
};

}
