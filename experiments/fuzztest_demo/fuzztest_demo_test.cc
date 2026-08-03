#include "experiments/fuzztest_demo/fuzztest_demo.h"
#include <algorithm>
#include <fuzztest/fuzztest_core.h>
#include <gtest/gtest.h>
#include <string>

namespace fuzztest_demo {

TEST(FuzzTestDemo, IsCommutative) { EXPECT_EQ(add(2, 3), add(3, 2)); }

void AddingZeroIsIdentity(int n) { EXPECT_EQ(add(n, 0), n); }
FUZZ_TEST(FuzzTestDemo, AddingZeroIsIdentity);

void ReverseIsItsOwnInverse(const std::string& s) { EXPECT_EQ(reverse(reverse(s)), s); }
FUZZ_TEST(FuzzTestDemo, ReverseIsItsOwnInverse);

void StringWithItsReverseIsAPalindrome(const std::string& s) {
    EXPECT_TRUE(is_palindrome(s + reverse(s)));
}
FUZZ_TEST(FuzzTestDemo, StringWithItsReverseIsAPalindrome);

void AverageIsBetweenInputs(int a, int b) {
    const int lo = std::min(a, b);
    const int hi = std::max(a, b);
    const int avg = average(a, b);
    EXPECT_GE(avg, lo);
    EXPECT_LE(avg, hi);
}
FUZZ_TEST(FuzzTestDemo, AverageIsBetweenInputs);

void EncodeDecodeNewlinesRoundtrips(const std::string& s) {
    EXPECT_EQ(decode_newlines(encode_newlines(s)), s);
}
FUZZ_TEST(FuzzTestDemo, EncodeDecodeNewlinesRoundtrips);

}  // namespace fuzztest_demo
