#include "base/check.h"
#include <format>
#include <iostream>

namespace base::internal {

namespace {
[[noreturn]] void immediate_crash() {
// We can't use abort() on Windows because it results in the abort/retry/ignore dialog which
// disrupts automated tests.
#if BUILDFLAG(IS_WIN)
    __debugbreak();
#elif BUILDFLAG(IS_POSIX)
    __builtin_trap();
    __builtin_unreachable();
#else
    // Last resort on unknown architectures. Should never be needed.
    std::abort();
#endif
}
}  // namespace

void check_fail(const char* expr, Location loc) {
    auto msg = std::format("[{}:{}] Check failed: {}", loc.file_name(), loc.line_number(), expr);
    std::cerr << msg << '\n';
    immediate_crash();
}

void notreached_fail(Location loc) {
    auto msg = std::format("[{}:{}] Unreachable code hit.", loc.file_name(), loc.line_number());
    std::cerr << msg << '\n';
    immediate_crash();
}

}  // namespace base::internal
