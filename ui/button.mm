#include "ui/button.h"
#include <Cocoa/Cocoa.h>
#include <utility>

namespace ui {

struct Button::State {
    std::string title;
    Button* owner = nullptr;
    platform::TaskScope task_scope;
    std::function<void()> click_handler;
    std::function<corral::Task<void>(Button&)> task_handler;
    NSButton* control = nil;
    NSTextField* status_label = nil;
    NSStackView* row = nil;
};

}  // namespace ui

@interface SimpleTextUIButtonDispatcher : NSObject
+ (instancetype)shared;
- (void)buttonPressed:(id)sender;
@end

@implementation SimpleTextUIButtonDispatcher

+ (instancetype)shared {
    static SimpleTextUIButtonDispatcher* dispatcher = [SimpleTextUIButtonDispatcher new];
    return dispatcher;
}

- (void)buttonPressed:(id)sender {
    auto* state = reinterpret_cast<ui::Button::State*>([sender tag]);
    if (!state) {
        return;
    }
    if (state->click_handler) {
        state->click_handler();
    }
    if (state->task_handler) {
        state->task_scope.start(
            [state]() -> corral::Task<void> { co_await state->task_handler(*state->owner); });
    }
}

@end

namespace {

NSStackView* ensure_stack_view(NSWindow* window) {
    NSView* content = window.contentView;
    NSStackView* existing = nil;
    for (NSView* subview in content.subviews) {
        if ([subview isKindOfClass:NSStackView.class]) {
            existing = static_cast<NSStackView*>(subview);
            break;
        }
    }
    if (existing) {
        return existing;
    }

    NSView* root = [[NSView alloc] initWithFrame:window.contentView.frame];
    root.translatesAutoresizingMaskIntoConstraints = NO;
    NSStackView* stack = [NSStackView stackViewWithViews:@[]];
    stack.orientation = NSUserInterfaceLayoutOrientationVertical;
    stack.alignment = NSLayoutAttributeLeading;
    stack.spacing = 12;
    stack.edgeInsets = NSEdgeInsetsMake(24, 24, 24, 24);
    stack.translatesAutoresizingMaskIntoConstraints = NO;

    [root addSubview:stack];
    [NSLayoutConstraint activateConstraints:@[
        [stack.leadingAnchor constraintEqualToAnchor:root.leadingAnchor],
        [stack.trailingAnchor constraintLessThanOrEqualToAnchor:root.trailingAnchor],
        [stack.topAnchor constraintEqualToAnchor:root.topAnchor],
    ]];

    window.contentView = root;
    return stack;
}

NSTextField* create_status_label() {
    NSTextField* label = [NSTextField labelWithString:@""];
    label.textColor = [NSColor secondaryLabelColor];
    label.hidden = YES;
    return label;
}

}  // namespace

namespace ui {

Button::Button(std::string title) : state_(std::make_unique<State>()) {
    state_->title = std::move(title);
    state_->owner = this;
}

Button::~Button() = default;

void Button::add_to(platform::Window& window) {
    NSWindow* native_window = (__bridge NSWindow*)window.native_handle();
    if (!native_window) {
        return;
    }

    state_->task_scope = window.task_scope();
    state_->control = [NSButton buttonWithTitle:@(state_->title.c_str())
                                         target:[SimpleTextUIButtonDispatcher shared]
                                         action:@selector(buttonPressed:)];
    state_->control.tag = reinterpret_cast<NSInteger>(state_.get());
    state_->status_label = create_status_label();
    state_->row = [NSStackView stackViewWithViews:@[ state_->control, state_->status_label ]];
    state_->row.orientation = NSUserInterfaceLayoutOrientationHorizontal;
    state_->row.alignment = NSLayoutAttributeCenterY;
    state_->row.spacing = 12;

    [ensure_stack_view(native_window) addArrangedSubview:state_->row];
}

void Button::on_click(std::function<void()> handler) {
    state_->click_handler = std::move(handler);
}

void Button::on_click_task(std::function<corral::Task<void>(Button&)> handler) {
    state_->task_handler = std::move(handler);
}

void Button::set_status_text(std::string text) {
    if (!state_->status_label) {
        return;
    }

    state_->status_label.stringValue = @(text.c_str());
    state_->status_label.hidden = text.empty();
}

}  // namespace ui
