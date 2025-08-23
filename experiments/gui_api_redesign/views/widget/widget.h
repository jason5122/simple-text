#pragma once

#include "experiments/gui_api_redesign/events/event_source.h"
#include "experiments/gui_api_redesign/views/focus/focus_manager.h"
#include <memory>

namespace views {

class View;

namespace internal {
class RootView;
}

class Widget : public ui::EventSource {
public:
    Widget();
    explicit Widget(InitParams params);
    ~Widget() override;
    Widget(const Widget&) = delete;
    Widget& operator=(const Widget&) = delete;

    // Returns the top level widget in a hierarchy (see is_top_level() for
    // the definition of top level widget.) Will return NULL if called
    // before the widget is attached to the top level widget's hierarchy.
    //
    // If you want to get the absolute primary application window, accounting for
    // e.g. bubble and menu anchoring, use GetPrimaryWindowWidget() instead.
    Widget* GetTopLevelWidget();
    const Widget* GetTopLevelWidget() const;

    View* GetRootView();
    const View* GetRootView() const;

    // Returns the FocusManager for this widget.
    // Note that all widgets in a widget hierarchy share the same focus manager.
    FocusManager* GetFocusManager();
    const FocusManager* GetFocusManager() const;

    // Returns the parent of this widget. Note that
    // * A top-level widget is not necessarily the root and may have a parent.
    // * A child widget shares the same visual style, e.g. the dark/light theme,
    //   with its parent.
    // * The native widget may change a widget's parent.
    // * The native view's parent might or might not be the parent's native view.
    // * For a desktop widget with a non-desktop parent, this value might be
    //   nullptr during shutdown.
    Widget* parent() { return parent_.get(); }
    const Widget* parent() const { return parent_.get(); }

    // True if the widget is considered top level widget. Top level widget
    // is a widget of TYPE_WINDOW, TYPE_PANEL, TYPE_WINDOW_FRAMELESS, BUBBLE,
    // POPUP or MENU, and has a focus manager and input method object associated
    // with it. TYPE_CONTROL and TYPE_TOOLTIP is not considered top level.
    bool is_top_level() const { return is_top_level_; }

    // Overridden from ui::EventSource:
    ui::EventSink* GetEventSink() override;

protected:
    // Creates the RootView to be used within this Widget. Subclasses may override
    // to create custom RootViews that do specialized event processing.
    // TODO(beng): Investigate whether or not this is needed.
    virtual internal::RootView* CreateRootView();

    // Provided to allow the NativeWidget implementations to destroy the RootView
    // _before_ the focus manager/tooltip manager.
    // TODO(beng): remove once we fold those objects onto this one.
    void DestroyRootView();

private:
    // The parent of this widget. This is the widget that associates with
    // the |params.parent| supplied to Init(). If no parent is given or the native
    // view parent has no associating Widget, this value will be nullptr.
    // For a desktop widget with a non-desktop parent, this value might be nullptr
    // during shutdown.
    base::WeakPtr<Widget> parent_ = nullptr;

    // The root of the View hierarchy attached to this window.
    // WARNING: see warning in tooltip_manager_ for ordering dependencies with
    // this and tooltip_manager_.
    std::unique_ptr<internal::RootView> root_view_;

    // The focus manager keeping track of focus for this Widget and any of its
    // children.  NULL for non top-level widgets.
    // WARNING: RootView's destructor calls into the FocusManager. As such, this
    // must be destroyed AFTER root_view_. This is enforced in DestroyRootView().
    std::unique_ptr<FocusManager> focus_manager_;

    // See |is_top_level()| accessor.
    bool is_top_level_ = false;

    // If a descendent of |root_view_| is focused, then clear the focus.
    void ClearFocusFromWidget();
};

}  // namespace views
