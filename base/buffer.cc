#include "buffer.h"
#include <cstdint>

namespace base {

void Buffer::setContents(std::string text) {
    data.clear();

    std::string line;
    for (char ch : text) {
        if (ch == '\n') {
            data.push_back(line);
            line.clear();
        } else {
            line += ch;
        }
    }
    data.push_back(line);

    flat_string = text;
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

std::string Buffer::getLineContent(size_t line_index) const {
    return data.at(line_index);
}

size_t Buffer::byteOfLine(size_t line_index) {
    size_t byte_offset = 0;
    for (uint16_t row = 0; row < line_index; row++) {
        byte_offset += data.at(row).size();
        byte_offset++;  // Include newline.
    }
    return byte_offset;
}

void Buffer::insert(size_t line_index, size_t line_offset, std::string_view text) {
    if (text == "\n" || text == "\r") {
        std::string before_lf = data.at(line_index).substr(0, line_offset);
        std::string after_lf = data.at(line_index).substr(line_offset);

        data.insert(data.begin() + line_index + 1, after_lf);
        data.at(line_index) = before_lf;
    } else {
        data.at(line_index).insert(line_offset, text);
    }
}

void Buffer::remove(size_t line_index, size_t line_offset, size_t bytes) {
    if (line_offset == data.at(line_index).length()) {
        if (line_index < lineCount() - 1) {
            data.at(line_index) += data.at(line_index + 1);
            data.erase(data.begin() + line_index + 1);
        }
    } else {
        data.at(line_index).erase(line_offset, bytes);
    }
}

void Buffer::backspace(size_t line_index, size_t line_offset, size_t bytes) {
    if (line_offset == 0) {
        if (line_index > 0) {
            data.at(line_index - 1) += data.at(line_index);
            data.erase(data.begin() + line_index);
        }
    } else {
        data.at(line_index).erase(line_offset - bytes, bytes);
    }
}

void Buffer::erase(size_t start_byte, size_t end_byte) {
    flat_string.erase(flat_string.begin() + start_byte, flat_string.begin() + end_byte);
    setContents(flat_string);
}

}
