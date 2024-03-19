#include "buffer.h"
#include <cstdint>
#include <sstream>

void Buffer::setContents(std::string txt) {
    std::stringstream ss(txt);
    std::string line;
    while (std::getline(ss, line, '\n')) {
        data.push_back(line);
    }
}

size_t Buffer::size() {
    size_t byte_count = 0;
    for (const std::string& line : data) {
        byte_count += line.size();
        byte_count += 1;  // Account for \n characters.
    }
    return byte_count;
}

size_t Buffer::lineCount() {
    return data.size();
}

void Buffer::getLineContent(std::string* buf, size_t line_index) const {
    *buf = data[line_index];
}

size_t Buffer::byteOfLine(size_t line_index) {
    size_t byte_offset = 0;
    for (uint16_t row = 0; row < line_index; row++) {
        byte_offset += data[row].size();
        byte_offset++;  // Include newline.
    }
    return byte_offset;
}

void Buffer::insert(size_t line_index, size_t line_offset, std::string_view txt) {
    data[line_index].insert(line_offset, txt);
}

void Buffer::debugInfo() {
    fprintf(stderr, "size = %zu, lineCount = %zu\n", size(), lineCount());
}
