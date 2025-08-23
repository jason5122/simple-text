#include "experiments/gui_api_redesign/views/widget/root_view.h"

namespace views::internal {

RootView::RootView(Widget* widget) : widget_(widget) {}

RootView::~RootView() {
    // If we have children remove them explicitly so to make sure a remove
    // notification is sent for each one of them.
    remove_all_child_views();
}

ui::EventTarget* RootView::GetRootForEvent(ui::Event* event) { return this; }

ui::EventTargeter* RootView::GetDefaultEventTargeter() { return this->GetEventTargeter(); }

const Widget* RootView::get_widget() const { return widget_; }

Widget* RootView::get_widget() {
    return const_cast<Widget*>(const_cast<const RootView*>(this)->get_widget());
}

}  // namespace views::internal
