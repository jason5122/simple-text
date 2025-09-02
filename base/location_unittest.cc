#include "base/location.h"
#include <gtest/gtest.h>

namespace base {

TEST(LocationTest, Current) {
    auto loc = Location::current();
    EXPECT_STREQ(loc.function_name(), "TestBody");
    EXPECT_STREQ(loc.file_name(), "base/location_unittest.cc");
    EXPECT_EQ(loc.line_number(), 7);
}

}  // namespace base
