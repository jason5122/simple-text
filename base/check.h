#pragma once

#include "base/compiler_specific.h"
#include "base/location.h"
#include "build/build_config.h"
#include <cstdio>
#include <cstdlib>

#define CHECK(cond)                                                                               \
    do {                                                                                          \
        if (!(cond)) [[unlikely]] {                                                               \
            ::base::internal::check_fail(#cond, ::base::Location::current());                     \
        }                                                                                         \
    } while (false)

namespace base::internal {

[[noreturn]] ALWAYS_INLINE void immediate_crash() {
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

[[noreturn]] inline void check_fail(const char* expr, Location loc) {
    std::fprintf(stderr, "[%s:%u] Check failed: %s\n", loc.file_name(), loc.line_number(), expr);
    std::fflush(stderr);
    immediate_crash();
}

}  // namespace base::internal
