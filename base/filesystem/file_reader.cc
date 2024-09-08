#include "file_reader.h"

// Debug use; remove this.
#include <iostream>

namespace base {

std::string ReadFile(std::string_view file_name) {
    std::string contents;

    FILE* fp = fopen(file_name.data(), "rb");
    if (!fp) {
        std::cerr << "ReadFile() error: file pointer is null\n";
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

}
