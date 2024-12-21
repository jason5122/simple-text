#include "file_reader.h"

// Debug use; remove this.
#include <fmt/base.h>

namespace base {

std::string ReadFile(std::string_view file_name) {
    std::string contents;

    FILE* fp = fopen(file_name.data(), "rb");
    if (!fp) {
        fmt::println("ReadFile() error: file pointer is null");
        std::abort();
    }

    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    contents.resize(size);
    rewind(fp);
    fread(&contents[0], 1, size, fp);
    fclose(fp);
    return contents;
}

void WriteFile(std::string_view file_name, std::string_view contents) {
    FILE* fp = fopen(file_name.data(), "wb");
    fwrite(&contents[0], 1, contents.length(), fp);
    fclose(fp);
}

std::unique_ptr<char[]> ReadFileBinary(std::string_view file_name, size_t& file_size) {
    FILE* file = fopen(file_name.data(), "rb");
    fseek(file, 0L, SEEK_END);
    file_size = ftell(file);
    fseek(file, 0L, SEEK_SET);

    auto data = std::make_unique<char[]>(file_size);
    fread(data.get(), file_size, 1, file);
    fclose(file);
    return data;
}

}  // namespace base
