#include <gtest/gtest.h>

#include "base/hash/hash.h"
#include "base/numeric/literals.h"

#include <cstddef>

namespace base {

TEST(HashCombineTest, Basic) {
    for (size_t i = 0; i < 100; ++i) {
        for (size_t j = 0; j < 100; ++j) {
            size_t hash = hash_combine(i, j);
            EXPECT_NE(hash, i);
            EXPECT_NE(hash, j);
            EXPECT_NE(hash, 0_Z);
        }
    }
}

}  // namespace base
