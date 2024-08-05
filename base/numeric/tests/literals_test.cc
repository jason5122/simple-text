#include "base/numeric/literals.h"
#include "gtest/gtest.h"
#include <type_traits>

TEST(LiteralsTest, TypeTraits) {
    auto sz = 5_Z;
    bool is_size_t = std::is_same<decltype(sz), std::size_t>::value;
    EXPECT_TRUE(is_size_t);
}
