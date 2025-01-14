#include <gtest/gtest.h>

#include "base/filesystem/file_path.h"

namespace base {

TEST(FilePathTest, IsEmpty) {
    FilePath empty_path;
    EXPECT_TRUE(empty_path.empty());

    FilePath path{"/this/is/a/path"};
    EXPECT_FALSE(path.empty());
    path.clear();
    EXPECT_TRUE(path.empty());
}

}  // namespace base
