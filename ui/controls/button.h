#pragma once

#include "ui/view.h"
#include <functional>
#include <string>

namespace ui {

class Button final : public View {
public:
    explicit Button(std::string text = {});

    void set_text(std::string text);
    void on_click(std::function<void()> callback);

    Size preferred_size(Size available) const override;
    void paint(PaintContext& context) override;
    void on_pointer_enter() override;
    void on_pointer_exit() override;
    bool on_pointer_down(const platform::PointerInfo& pointer_info) override;
    bool on_pointer_up(const platform::PointerInfo& pointer_info) override;

protected:
    bool accepts_pointer() const override { return true; }

private:
    std::string text_;
    std::function<void()> on_click_;
    bool hovered_ = false;
    bool pressed_ = false;
};

}  // namespace ui
