#pragma once

#include "experiments/gui_api_redesign/events/event_processor.h"
#include "experiments/gui_api_redesign/views/view.h"

namespace views {

class Widget;

namespace internal {

class RootView : public View, public ui::EventProcessor {
public:
    explicit RootView(Widget* widget);
    ~RootView() override;
    RootView(const RootView&) = delete;
    RootView& operator=(const RootView&) = delete;

    // ui::EventProcessor:
    ui::EventTarget* GetRootForEvent(ui::Event* event) override;
    ui::EventTargeter* GetDefaultEventTargeter() override;

    // View:
    const Widget* get_widget() const override;
    Widget* get_widget() override;

private:
    const Widget* widget_;
};

}  // namespace internal

}  // namespace views
