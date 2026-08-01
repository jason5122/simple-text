#include "experiments/fuzztest_demo/fuzztest_demo.h"
#include <fuzztest/fuzztest_core.h>
#include <gtest/gtest.h>
#include <string>

namespace fuzztest_demo {
namespace {

// A plain GoogleTest test still works alongside fuzz tests.
TEST(AddTest, IsCommutative) { EXPECT_EQ(Add(2, 3), Add(3, 2)); }

// Property: adding zero returns the original value. FuzzTest will try many
// values of `n` (and mutate toward interesting ones like INT_MIN/INT_MAX).
void AddingZeroIsIdentity(int n) { EXPECT_EQ(Add(n, 0), n); }
FUZZ_TEST(AddTest, AddingZeroIsIdentity);

// Property: reversing a string twice yields the original string.
void ReverseIsItsOwnInverse(const std::string& s) { EXPECT_EQ(Reverse(Reverse(s)), s); }
FUZZ_TEST(ReverseTest, ReverseIsItsOwnInverse);

// Property: a string followed by its reverse is always a palindrome.
void StringWithItsReverseIsAPalindrome(const std::string& s) {
    EXPECT_TRUE(IsPalindrome(s + Reverse(s)));
}
FUZZ_TEST(PalindromeTest, StringWithItsReverseIsAPalindrome);

}  // namespace
}  // namespace fuzztest_demo
