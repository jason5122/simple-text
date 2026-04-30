#include "platform/task.h"
#include "platform/task_internal.h"
#include <utility>

namespace platform {

TaskScope::TaskScope(std::shared_ptr<internal::TaskScopeState> state) : state_(std::move(state)) {}

void TaskScope::cancel() const {
    if (state_) {
        state_->cancel();
    }
}

void TaskScope::start_impl(std::function<corral::Task<void>()> task_factory) const {
    if (state_) {
        state_->start(std::move(task_factory));
    }
}

}  // namespace platform
