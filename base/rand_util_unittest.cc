#include "base/rand_util.h"
#include <gtest/gtest.h>

namespace base {

TEST(RandUtilTest, RandBytes) {
    std::array<uint8_t, 64> a{}, b{};
    rand_bytes(a);
    rand_bytes(b);

    // Not all zeros (vanishingly unlikely: 256^-64).
    EXPECT_NE(std::count(a.begin(), a.end(), 0u), 64);

    // Two independent draws should differ (vanishingly unlikely: 256^-64).
    EXPECT_NE(0, std::memcmp(a.data(), b.data(), a.size()));
}

}  // namespace base
