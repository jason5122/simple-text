#include "base/check.h"
#include <format>
#include <iostream>

namespace base::internal {

void check_fail(const char* expr, Location loc) {
    auto msg = std::format("[{}:{}] Check failed: {}", loc.file_name(), loc.line_number(), expr);
    std::cerr << msg << std::endl;
    __builtin_trap();
}

void notreached_fail(Location loc) {
    auto msg = std::format("[{}:{}] Unreachable code hit.", loc.file_name(), loc.line_number());
    std::cerr << msg << std::endl;
    __builtin_trap();
}

}  // namespace base::internal
