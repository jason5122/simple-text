#pragma once

#include "experiments/gui_api_redesign/events/event_handler.h"
#include "experiments/gui_api_redesign/events/event_target.h"
#include "experiments/gui_api_redesign/events/event_targeter.h"
#include "experiments/gui_api_redesign/views/view_targeter.h"
#include "experiments/gui_api_redesign/views/widget/widget.h"
#include <vector>

namespace views {

class View : public ui::EventTarget, public ui::EventHandler {
public:
    using Views = std::vector<View*>;

    View();
    ~View() override;
    View(const View&) = delete;
    View& operator=(const View&) = delete;

    // Get the Widget that hosts this View, if any.
    virtual const Widget* get_widget() const;
    virtual Widget* get_widget();

    void remove_child_view(View* view);
    void remove_all_child_views();

    const Views& children() const { return children_; }
    const View* parent() const { return parent_; }
    View* parent() { return parent_; }
    Views::const_iterator find_child(const View* view) const;

    // Overridden from ui::EventTarget:
    bool can_accept_event(const ui::Event& event) override;
    EventTarget* get_parent_target() override;
    std::unique_ptr<ui::EventTargetIterator> get_child_iterator() const override;
    ui::EventTargeter* GetEventTargeter() override;

private:
    // The widget that this view is attached to. This is null if the view is not attached to a
    // widget.
    Widget* widget_ = nullptr;
    View* parent_ = nullptr;
    Views children_;
    std::unique_ptr<ViewTargeter> targeter_;

    void do_remove_child_view(View* view,
                              bool update_tool_tip,
                              bool delete_removed_view,
                              View* new_parent);
};

}  // namespace views
