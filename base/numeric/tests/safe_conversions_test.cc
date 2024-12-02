#include "base/numeric/safe_conversions.h"

#include <gtest/gtest.h>

namespace base {

TEST(SafeConversionsTest, StrictCast) {
    size_t size [[maybe_unused]];

    uint8_t u_i = 10;
    size = strict_cast<size_t>(u_i);

    // This should not compile.
    // int i = 10;
    // size = strict_cast<size_t>(i);
}

TEST(SafeConversionsTest, CheckedCast) {
    size_t size [[maybe_unused]];

    uint8_t u_i = 10;
    size = checked_cast<size_t>(u_i);

    int i = 10;
    size = checked_cast<size_t>(i);
}

}  // namespace base
