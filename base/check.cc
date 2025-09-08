#include "base/check.h"
#include <fmt/base.h>

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
    fmt::println(stderr, "[{}:{}] Check failed: {}", loc.file_name(), loc.line_number(), expr);
    immediate_crash();
}

void notreached_fail(Location loc) {
    fmt::println(stderr, "[{}:{}] Unreachable code hit.", loc.file_name(), loc.line_number());
    immediate_crash();
}

}  // namespace base::internal
