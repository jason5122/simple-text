#include "base/location.h"
#include "build/build_config.h"
#include <gtest/gtest.h>

namespace base {

TEST(LocationTest, Current) {
    int line = __LINE__ + 1;
    auto loc = Location::current();
    EXPECT_STREQ(loc.function_name(), "TestBody");

#if BUILDFLAG(IS_WIN)
    EXPECT_STREQ(loc.file_name(), "base\\location_unittest.cc");
#else
    EXPECT_STREQ(loc.file_name(), "base/location_unittest.cc");
#endif

    EXPECT_EQ(loc.line_number(), line);
}

}  // namespace base
