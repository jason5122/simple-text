#pragma once

#include "platform/window.h"
#include "ui/view.h"
#include <memory>

namespace ui {

class RootViewHost final : public platform::WindowDelegate {
public:
    explicit RootViewHost(std::unique_ptr<View> root);

    View& root() { return *root_; }

    void on_draw(platform::Window& window,
                 gfx::Frame& frame,
                 const platform::FrameInfo& frame_info) override;
    void on_resize(platform::Window& window, const platform::ResizeInfo& resize_info) override;
    void on_scroll(platform::Window& window, const platform::ScrollInfo& scroll_info) override;
    void on_pointer_move(platform::Window& window,
                         const platform::PointerInfo& pointer_info) override;
    void on_pointer_down(platform::Window& window,
                         const platform::PointerInfo& pointer_info) override;
    void on_pointer_up(platform::Window& window,
                       const platform::PointerInfo& pointer_info) override;

private:
    Point point_from_pointer(const platform::PointerInfo& pointer_info) const;
    void ensure_layout();
    void set_hovered(View* view);

    std::unique_ptr<View> root_;
    platform::Window* window_ = nullptr;
    Size size_;
    bool needs_layout_ = true;
    View* hovered_ = nullptr;
    View* pressed_ = nullptr;
};

}  // namespace ui
