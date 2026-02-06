#include "experiments/message_loop/message_pump.h"
#include <AppKit/AppKit.h>
#include <iostream>
#include <print>
#include <queue>
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

    NSRect frame = NSMakeRect(0, 600, 1200, 600);
    NSUInteger style =
        NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable;

    NSWindow* window = [[NSWindow alloc] initWithContentRect:frame
                                                   styleMask:style
                                                     backing:NSBackingStoreBuffered
                                                       defer:NO];

    NSApp.activationPolicy = NSApplicationActivationPolicyRegular;
    [window makeKeyAndOrderFront:nil];
}

class TaskRunner : public MessagePump::Delegate {
public:
    using Task = std::function<void()>;

    // Post an immediate task.
    void post_task(Task task) { tasks_.push(std::move(task)); }

    NextWorkInfo do_work() override {
        if (tasks_.empty()) return {base::TimeTicks::max()};

        auto task = std::move(tasks_.front());
        tasks_.pop();

        if (task) task();

        // Keep your existing pacing behavior (1 task per second).
        return {base::TimeTicks::now() + base::TimeDelta::seconds(1)};
    }

private:
    std::queue<Task> tasks_;
};

}  // namespace

int main() {
    example_appkit_stuff();

    TaskRunner task_runner;
    task_runner.post_task([]() { std::println(std::cerr, "task 1"); });
    task_runner.post_task([]() { std::println(std::cerr, "task 2"); });
    task_runner.post_task([]() { std::println(std::cerr, "task 3"); });
    task_runner.post_task([]() { std::println(std::cerr, "task 4"); });
    task_runner.post_task([]() { std::println(std::cerr, "task 5"); });

    auto ui_message_pump = MessagePump::create();
    ui_message_pump->run(&task_runner);
}
