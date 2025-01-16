#pragma once

#include <memory>

#include <stdio.h>

namespace base {

namespace internal {

// Functor for `ScopedFILE` (below).
struct ScopedFILECloser {
    inline void operator()(FILE* x) const {
        if (x) {
            fclose(x);
        }
    }
};

}  // namespace internal

// Automatically closes `FILE*`s.
using ScopedFILE = std::unique_ptr<FILE, internal::ScopedFILECloser>;

}  // namespace base
