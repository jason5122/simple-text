#include "base/check.h"
#include <print>

namespace base::internal {

void check_fail(const char* expr, Location loc) {
    std::println(stderr, "[{}:{}] Check failed: {}", loc.file_name(), loc.line_number(), expr);
    __builtin_trap();
}

void notreached_fail(Location loc) {
    std::println(stderr, "[{}:{}] Unreachable code hit.", loc.file_name(), loc.line_number());
    __builtin_trap();
}

}  // namespace base::internal
