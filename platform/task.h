#pragma once

#include <chrono>
#include <corral/corral.h>
#include <functional>
#include <memory>
#include <utility>

namespace platform {

namespace internal {
struct TaskScopeState;
}  // namespace internal

class TaskScope {
public:
    TaskScope() = default;

    explicit operator bool() const { return !!state_; }

    void cancel() const;

    template <class Fn>
    void start(Fn&& fn) const {
        start_impl(std::function<corral::Task<void>()>(std::forward<Fn>(fn)));
    }

private:
    explicit TaskScope(std::shared_ptr<internal::TaskScopeState> state);
    void start_impl(std::function<corral::Task<void>()> task_factory) const;

    std::shared_ptr<internal::TaskScopeState> state_;

    friend class App;
    friend class Window;
    friend class AppMac;
    friend class WindowMac;
};

corral::Task<void> resume_on_ui();
corral::Task<void> sleep_for(std::chrono::milliseconds delay);

}  // namespace platform
