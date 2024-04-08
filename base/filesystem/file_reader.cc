#include "file_reader.h"
#include <fstream>

std::string ReadFile(fs::path file_name) {
    std::ifstream in(file_name);
    std::string contents((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    return contents;
}
