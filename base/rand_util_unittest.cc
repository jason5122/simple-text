#include "base/rand_util.h"
#include <gtest/gtest.h>

namespace base {

TEST(RandUtilTest, RandInt) {
    for (size_t i = 0; i < 100; ++i) {
        int k = rand_int(0, 10);
        EXPECT_GE(k, 0);
        EXPECT_LE(k, 10);
    }
}

TEST(RandUtilTest, RandDouble) {
    for (size_t i = 0; i < 100; ++i) {
        EXPECT_LT(rand_double(), 1.0);
    }
}

TEST(RandUtilTest, RandFloat) {
    for (size_t i = 0; i < 100; ++i) {
        EXPECT_LT(rand_float(), 1.0);
    }
}

TEST(RandUtilTest, RandBytes) {
    std::array<uint8_t, 64> a{}, b{};
    rand_bytes(a);
    rand_bytes(b);

    // Not all zeros (vanishingly unlikely: 256^-64).
    EXPECT_NE(std::count(a.begin(), a.end(), 0u), 64);

    // Two independent draws should differ (vanishingly unlikely: 256^-64).
    EXPECT_NE(0, std::memcmp(a.data(), b.data(), a.size()));
}

TEST(RandUtilTest, RandBytesAsString) {
    for (size_t len = 1; len <= 20; ++len) {
        EXPECT_EQ(rand_bytes_as_string(len).length(), len);
    }
}

TEST(RandUtilTest, RandBytesAsVector) {
    for (size_t len = 1; len <= 20; ++len) {
        EXPECT_EQ(rand_bytes_as_vector(len).size(), len);
    }
}

}  // namespace base
