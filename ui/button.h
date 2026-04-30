#pragma once

#include "platform/window.h"
#include <corral/corral.h>
#include <functional>
#include <memory>
#include <string>

namespace ui {

class Button {
public:
    struct State;

    explicit Button(std::string title);
    ~Button();

    Button(const Button&) = delete;
    Button& operator=(const Button&) = delete;

    void add_to(platform::Window& window);
    void on_click(std::function<void()> handler);
    void on_click_task(std::function<corral::Task<void>(Button&)> handler);
    void set_status_text(std::string text);

private:
    std::unique_ptr<State> state_;
};

}  // namespace ui
