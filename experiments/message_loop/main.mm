#include "experiments/message_loop/message_pump.h"
#include <AppKit/AppKit.h>
#include <spdlog/spdlog.h>

namespace {
void example_appkit_stuff() {
    [NSApplication sharedApplication];

    NSMenu* main_menu = [[NSMenu alloc] initWithTitle:@""];
    NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:@"" action:nil keyEquivalent:@""];
    NSMenu* submenu = [[NSMenu alloc] initWithTitle:@""];
    [submenu addItem:[[NSMenuItem alloc] initWithTitle:@"Quit"
                                                action:@selector(terminate:)
                                         keyEquivalent:@"q"]];
    item.submenu = submenu;
    [main_menu addItem:item];
    NSApp.mainMenu = main_menu;

    NSRect frame = NSMakeRect(0, 0, 1200, 800);
    NSUInteger style =
        NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable;

    NSWindow* window = [[NSWindow alloc] initWithContentRect:frame
                                                   styleMask:style
                                                     backing:NSBackingStoreBuffered
                                                       defer:NO];

    NSApp.activationPolicy = NSApplicationActivationPolicyRegular;
    [window makeKeyAndOrderFront:nil];
}

class WorkSource : public MessagePump::Delegate {
public:
    NextWorkInfo do_work() override {
        spdlog::info("hello");

        auto t = base::TimeTicks::now() + base::TimeDelta::seconds(1);
        return {.delayed_run_time = t};
    }
};

}  // namespace

int main() {
    example_appkit_stuff();

    WorkSource work_source;

    auto message_pump = MessagePump::create();
    message_pump->run(&work_source);
}
