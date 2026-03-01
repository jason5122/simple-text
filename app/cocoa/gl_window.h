#pragma once

#include "app/window.h"
#include <Cocoa/Cocoa.h>
#include <memory>

namespace app {

class GLWindow final : public Window {
public:
    static std::unique_ptr<Window> create(const WindowOptions& options);
    void set_title(std::string_view title) override;

private:
    GLWindow(const WindowOptions& options);

    NSWindow* ns_window;
};

}  // namespace app
