#pragma once

#include <corral/corral.h>
#include <functional>
#include <utility>
#include <vector>

namespace platform::internal {

struct TaskScopeState {
    corral::Nursery* nursery = nullptr;
    bool cancel_requested = false;
    std::vector<std::function<corral::Task<void>()>> pending_tasks;

    void bind(corral::Nursery& bound_nursery) {
        nursery = &bound_nursery;
        flush_pending_tasks();
        if (cancel_requested) {
            nursery->cancel();
        }
    }

    void unbind(corral::Nursery& bound_nursery) {
        if (nursery == &bound_nursery) {
            nursery = nullptr;
        }
    }

    void cancel() {
        cancel_requested = true;
        if (nursery) {
            nursery->cancel();
        }
    }

    void start(std::function<corral::Task<void>()> task_factory) {
        if (!nursery) {
            pending_tasks.push_back(std::move(task_factory));
            return;
        }

        nursery->start([task_factory = std::move(task_factory)]() mutable -> corral::Task<void> {
            co_await task_factory();
        });
    }

private:
    void flush_pending_tasks() {
        auto tasks = std::move(pending_tasks);
        for (auto& task_factory : tasks) {
            start(std::move(task_factory));
        }
    }
};

}  // namespace platform::internal
