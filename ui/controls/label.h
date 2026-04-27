#pragma once

#include "ui/view.h"
#include <string>
#include <string_view>

namespace ui {

class Label final : public View {
public:
    explicit Label(std::string text = {});

    void set_text(std::string_view text);
    Size preferred_size(Size available) const override;
    void paint(PaintContext& context) override;

private:
    std::string text_;
};

}  // namespace ui
