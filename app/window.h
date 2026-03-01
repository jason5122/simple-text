#pragma once

#include <string>
#include <string_view>

namespace app {

struct WindowOptions {
    int width;
    int height;
    std::string title;
};

class Window {
public:
    virtual ~Window();

    virtual void set_title(std::string_view title) = 0;
};

}  // namespace app
