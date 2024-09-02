#include "base/filesystem/file_reader.h"
#include <gtest/gtest.h>

TEST(FileReaderTest, ReadFile1) {
    std::string contents = ReadFile("hi.txt");
}

TEST(FileReaderTest, ReadFile2) {
    std::string contents;

    FILE* fp;
    fp = fopen("hi.txt", "rb");
    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    contents.resize(size);
    rewind(fp);
    fread(&contents[0], 1, size, fp);
    fclose(fp);
}
