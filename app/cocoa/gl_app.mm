#include "app/cocoa/app_controller.h"
#include "app/cocoa/gl_app.h"
#include "app/cocoa/gl_window.h"
#include "base/check.h"
#include "base/message_loop/message_pump.h"
#include <Cocoa/Cocoa.h>
#include <functional>
#include <print>
#include <queue>

namespace app {

GLApp::GLApp() = default;

std::unique_ptr<App> GLApp::create() {
    [NSApplication sharedApplication];
    // TODO: Call this in `applicationWillFinishLaunching` like Chromium.
    NSWindow.allowsAutomaticWindowTabbing = NO;

    NSMenu* main_menu = [[NSMenu alloc] initWithTitle:@""];
    NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:@"" action:nil keyEquivalent:@""];
    NSMenu* submenu = [[NSMenu alloc] initWithTitle:@""];
    [submenu addItem:[[NSMenuItem alloc] initWithTitle:@"Quit"
                                                action:@selector(terminate:)
                                         keyEquivalent:@"q"]];
    item.submenu = submenu;
    [main_menu addItem:item];
    NSApp.mainMenu = main_menu;

    return std::unique_ptr<App>(new GLApp());
}

namespace {
class TaskRunner : public base::MessagePump::Delegate {
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

void GLApp::run() {
    // Create the app delegate by requesting the shared AppController.
    CHECK_EQ(nil, NSApp.delegate);
    AppController* app_controller = AppController.sharedController;
    CHECK_NE(nil, NSApp.delegate);

    TaskRunner task_runner;
    task_runner.post_task([]() { std::println("task 1"); });
    task_runner.post_task([]() { std::println("task 2"); });
    task_runner.post_task([]() { std::println("task 3"); });
    task_runner.post_task([]() { std::println("task 4"); });
    task_runner.post_task([]() { std::println("task 5"); });

    auto ui_message_pump = base::MessagePump::create();
    ui_message_pump->run(&task_runner);
}

std::unique_ptr<Window> GLApp::create_window(const WindowOptions& options) {
    return std::unique_ptr<Window>(GLWindow::create(options));
}

}  // namespace app
