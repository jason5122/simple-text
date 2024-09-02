#include "file_reader.h"
#include <fstream>

std::string ReadFile(fs::path file_name) {
    static constexpr size_t kReadSize = 4096;

    std::ifstream stream{file_name};
    std::string out{};
    std::string buf(kReadSize, '\0');
    while (stream.read(&buf[0], kReadSize)) {
        out.append(buf, 0, stream.gcount());
    }
    out.append(buf, 0, stream.gcount());
    return out;
}
