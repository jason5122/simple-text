#pragma once

#include "platform/window.h"
#include "ui/paint_context.h"
#include "ui/types.h"
#include <functional>
#include <memory>
#include <vector>

namespace ui {

class View {
public:
    virtual ~View() = default;

    template <typename T>
    T* add_child(std::unique_ptr<T> child) {
        T* raw_child = child.get();
        child->parent_ = this;
        child->set_invalidation_callback(invalidate_);
        children_.push_back(std::move(child));
        invalidate();
        return raw_child;
    }

    virtual Size preferred_size(Size available) const;
    virtual void layout(Rect bounds);
    virtual void paint(PaintContext& context);

    View* hit_test(Point point);
    virtual void on_pointer_enter() {}
    virtual void on_pointer_exit() {}
    virtual bool on_pointer_move(const platform::PointerInfo& pointer_info);
    virtual bool on_pointer_down(const platform::PointerInfo& pointer_info);
    virtual bool on_pointer_up(const platform::PointerInfo& pointer_info);

    Rect bounds() const { return bounds_; }
    bool contains(Point point) const { return bounds_.contains(point); }
    void set_invalidation_callback(std::function<void()> invalidate);
    void invalidate();

protected:
    virtual bool accepts_pointer() const { return false; }

    const std::vector<std::unique_ptr<View>>& children() const { return children_; }
    std::vector<std::unique_ptr<View>>& children() { return children_; }

private:
    View* parent_ = nullptr;
    Rect bounds_;
    std::vector<std::unique_ptr<View>> children_;
    std::function<void()> invalidate_;
};

}  // namespace ui
