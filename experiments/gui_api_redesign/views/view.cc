#include "experiments/gui_api_redesign/events/event_target_iterator.h"
#include "experiments/gui_api_redesign/views/view.h"

namespace views {

View::View() { set_target_handler(this); }

View::~View() {
    if (parent_) {
        parent_->remove_child_view(this);
    }
}

const Widget* View::get_widget() const { return widget_; }

Widget* View::get_widget() {
    return const_cast<Widget*>(const_cast<const View*>(this)->get_widget());
}

void View::remove_child_view(View* view) { do_remove_child_view(view, true, false, nullptr); }

void View::remove_all_child_views() {
    while (!children_.empty()) {
        do_remove_child_view(children_.front(), false, true, nullptr);
    }
}

View::Views::const_iterator View::find_child(const View* view) const {
    return std::ranges::find(children_, view);
}

bool View::can_accept_event(const ui::Event& event) {
    // TODO: See if we should change this.
    return true;
}

ui::EventTarget* View::get_parent_target() { return parent_; }

std::unique_ptr<ui::EventTargetIterator> View::get_child_iterator() const {
    return std::make_unique<ui::EventTargetIteratorPtrImpl<View>>(children_);
}

ui::EventTargeter* View::GetEventTargeter() { return targeter_.get(); }

void View::do_remove_child_view(View* view,
                                bool update_tool_tip,
                                bool delete_removed_view,
                                View* new_parent) {
    const auto i = find_child(view);
    if (i == children_.cend()) {
        return;
    }

    std::unique_ptr<View> view_to_be_deleted;
    view->RemoveFromFocusList();

    Widget* widget = GetWidget();
    bool is_removed_from_widget = false;
    if (widget) {
        UnregisterChildrenForVisibleBoundsNotification(view);
        if (view->GetVisible()) {
            view->SchedulePaint();
        }

        is_removed_from_widget = !new_parent || new_parent->GetWidget() != widget;
        if (is_removed_from_widget) {
            widget->NotifyWillRemoveView(view);
        }
    }

    // Need to notify the layout manager because one of the callbacks below might
    // want to know the view's new preferred size, minimum size, etc.
    if (HasLayoutManager()) {
        GetLayoutManager()->ViewRemoved(this, view);
    }

    view->PropagateRemoveNotifications(this, new_parent, is_removed_from_widget);

    // Make sure the layers belonging to the subtree rooted at |view| get
    // removed. Only do this after all the removal notifications have gone out.
    view->OrphanLayers();
    if (widget) {
        widget->LayerTreeChanged();
    }

    if (view->parent_) {
        view->parent_->GetViewAccessibility().NotifyEvent(ax::mojom::Event::kChildrenChanged,
                                                          true);
    }

    view->parent_ = nullptr;

    if (delete_removed_view && !view->owned_by_client_) {
        view_to_be_deleted.reset(view);
    }

#if DCHECK_IS_ON()
    DCHECK(!iterating_);
#endif
    children_.erase(i);

    if (update_tool_tip) {
        UpdateTooltip();
    }

    observers_.Notify(&ViewObserver::OnChildViewRemoved, this, view);
}

}  // namespace views
