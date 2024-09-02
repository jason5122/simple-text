#include "file_reader.h"

namespace base {

std::string ReadFile(std::string_view file_name) {
    std::string contents;

    FILE* fp = fopen(file_name.data(), "rb");
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
