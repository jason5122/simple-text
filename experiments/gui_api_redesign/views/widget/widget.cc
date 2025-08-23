#include "experiments/gui_api_redesign/views/widget/root_view.h"
#include "experiments/gui_api_redesign/views/widget/widget.h"

namespace views {

Widget* Widget::GetTopLevelWidget() {
    return const_cast<Widget*>(static_cast<const Widget*>(this)->GetTopLevelWidget());
}

const Widget* Widget::GetTopLevelWidget() const {
    // GetTopLevelNativeWidget doesn't work during destruction because
    // property is gone after gobject gets deleted. Short circuit here
    // for toplevel so that InputMethod can remove itself from
    // focus manager.
    if (is_top_level()) {
        return this;
    }
    return native_widget_ ? native_widget_->GetTopLevelWidget() : nullptr;
}

View* Widget::GetRootView() { return root_view_.get(); }

const View* Widget::GetRootView() const { return root_view_.get(); }

FocusManager* Widget::GetFocusManager() {
    Widget* toplevel_widget = GetTopLevelWidget();
    return toplevel_widget ? toplevel_widget->focus_manager_.get() : nullptr;
}

const FocusManager* Widget::GetFocusManager() const {
    const Widget* toplevel_widget = GetTopLevelWidget();
    return toplevel_widget ? toplevel_widget->focus_manager_.get() : nullptr;
}

// Overridden from ui::EventSource:
ui::EventSink* Widget::GetEventSink() { return root_view_.get(); }

internal::RootView* Widget::CreateRootView() { return new internal::RootView(this); }

void Widget::DestroyRootView() {
    ClearFocusFromWidget();
    NotifyWillRemoveView(root_view_.get());
    non_client_view_ = nullptr;
    // Remove all children before the unique_ptr reset so that
    // GetWidget()->GetRootView() doesn't return nullptr while the views hierarchy
    // is being torn down.
    root_view_->remove_all_child_views();
    root_view_.reset();
}

void Widget::ClearFocusFromWidget() {
    FocusManager* focus_manager = GetFocusManager();
    // We are being removed from a window hierarchy.  Treat this as
    // the root_view_ being removed.
    if (focus_manager) {
        focus_manager->ViewRemoved(root_view_.get());
    }
}

}  // namespace views
