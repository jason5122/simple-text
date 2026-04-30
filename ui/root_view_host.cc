#include "ui/root_view_host.h"

namespace ui {

RootViewHost::RootViewHost(std::unique_ptr<View> root) : root_(std::move(root)) {
    root_->set_invalidation_callback([this] {
        if (window_) {
            window_->request_redraw();
        }
    });
}

void RootViewHost::on_draw(platform::Window& window,
                           gfx::Frame& frame,
                           const platform::FrameInfo& frame_info) {
    window_ = &window;
    size_ =
        Size{static_cast<float>(frame_info.width_px), static_cast<float>(frame_info.height_px)};
    ensure_layout();

    PaintContext context(frame);
    frame.clear({1.0f, 1.0f, 1.0f, 1.0f});
    root_->paint(context);
}

void RootViewHost::on_resize(platform::Window& window, const platform::ResizeInfo& resize_info) {
    window_ = &window;
    size_ =
        Size{static_cast<float>(resize_info.width_px), static_cast<float>(resize_info.height_px)};
    needs_layout_ = true;
    window.request_redraw();
}

void RootViewHost::on_scroll(platform::Window& window, const platform::ScrollInfo& scroll_info) {
    window_ = &window;
}

void RootViewHost::on_pointer_move(platform::Window& window,
                                   const platform::PointerInfo& pointer_info) {
    window_ = &window;
    ensure_layout();
    Point point = point_from_pointer(pointer_info);
    View* target = root_->hit_test(point);
    set_hovered(target);
    if (target && target->on_pointer_move(pointer_info)) {
        window.request_redraw();
    }
}

void RootViewHost::on_pointer_down(platform::Window& window,
                                   const platform::PointerInfo& pointer_info) {
    window_ = &window;
    ensure_layout();
    Point point = point_from_pointer(pointer_info);
    pressed_ = root_->hit_test(point);
    if (pressed_ && pressed_->on_pointer_down(pointer_info)) {
        window.request_redraw();
    }
}

void RootViewHost::on_pointer_up(platform::Window& window,
                                 const platform::PointerInfo& pointer_info) {
    window_ = &window;
    ensure_layout();
    View* target = pressed_ ? pressed_ : root_->hit_test(point_from_pointer(pointer_info));
    pressed_ = nullptr;
    if (target && target->on_pointer_up(pointer_info)) {
        window.request_redraw();
    }
}

Point RootViewHost::point_from_pointer(const platform::PointerInfo& pointer_info) const {
    return Point{pointer_info.x_px, pointer_info.y_px};
}

void RootViewHost::ensure_layout() {
    if (!needs_layout_) return;
    root_->layout(Rect{0, 0, size_.width, size_.height});
    needs_layout_ = false;
}

void RootViewHost::set_hovered(View* view) {
    if (hovered_ == view) return;
    if (hovered_) {
        hovered_->on_pointer_exit();
    }
    hovered_ = view;
    if (hovered_) {
        hovered_->on_pointer_enter();
    }
}

}  // namespace ui
