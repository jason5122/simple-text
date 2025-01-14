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

TEST(FilePathTest, DirName) {
    FilePath path1{"../a"};
    EXPECT_EQ(path1.DirName(), FilePath{".."});

    FilePath path2{"/path/to/file.txt"};
    EXPECT_EQ(path2.DirName(), FilePath{"/path/to"});
}

TEST(FilePathTest, BaseName) {
    FilePath path1{"../a"};
    EXPECT_EQ(path1.BaseName(), FilePath{"a"});

    FilePath path2{"/path/to/file.txt"};
    EXPECT_EQ(path2.BaseName(), FilePath{"file.txt"});
}

TEST(FilePathTest, Extension) {
    FilePath path1{"image.jpg"};
    EXPECT_EQ(path1.Extension(), ".jpg");

    FilePath path2{"/path/to/file.txt"};
    EXPECT_EQ(path2.Extension(), ".txt");
}

TEST(FilePathTest, RemoveExtension) {
    FilePath path{"/pics/jojo.jpg"};
    EXPECT_EQ(path.RemoveExtension(), FilePath{"/pics/jojo"});
}

}  // namespace base
