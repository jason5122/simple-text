#pragma once

#include "platform/app.h"
#include "platform/task.h"
#include <corral/corral.h>
#include <functional>
#include <memory>
#include <vector>

namespace platform {

namespace internal {
struct UiLoop;
}  // namespace internal

class WindowMac;

class AppMac final : public App {
public:
    static std::unique_ptr<App> create(AppOptions options);
    ~AppMac() override;

    Window* create_window(WindowOptions options, WindowDelegate* delegate) override;
    TaskScope task_scope() const override;
    int run() override;
    void quit() override;

    bool request_quit();
    void window_did_close(WindowMac* window);
    void window_requested_close(WindowMac* window);

private:
    explicit AppMac(AppOptions options);
    corral::Task<void> run_until_quit();

    AppOptions options_;
    std::unique_ptr<internal::UiLoop> ui_loop_;
    std::shared_ptr<internal::TaskScopeState> task_scope_;
    corral::Nursery* app_nursery_ = nullptr;
    bool quit_requested_ = false;
    std::function<void()> emit_quit_;
    std::vector<std::unique_ptr<WindowMac>> windows_;
    void* app_delegate_ = nullptr;
};

}  // namespace platform
