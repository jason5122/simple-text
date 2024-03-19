#include "buffer.h"
#include <iostream>

void Buffer::setContents(std::string txt) {
    std::string line;
    for (char ch : txt) {
        if (ch == '\n') {
            data.push_back(line);
            line.clear();
        } else {
            line += ch;
        }
    }
    data.push_back("");
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
    if (txt == "\n") {
        std::string before_lf = data[line_index].substr(0, line_offset);
        std::string after_lf = data[line_index].substr(line_offset);

        data.insert(data.begin() + line_index + 1, after_lf);
        data[line_index] = before_lf;
    } else {
        data[line_index].insert(line_offset, txt);
    }
}

void Buffer::remove(size_t line_index, size_t line_offset, size_t bytes) {
    if (line_offset == data[line_index].length()) {
        if (line_index < lineCount() - 1) {
            data[line_index] += data[line_index + 1];
            data.erase(data.begin() + line_index + 1);
        }
    } else {
        data[line_index].erase(line_offset, bytes);
    }
}

void Buffer::backspace(size_t line_index, size_t line_offset, size_t bytes) {
    if (line_offset == 0) {
        if (line_index > 0) {
            data[line_index - 1] += data[line_index];
            data.erase(data.begin() + line_index);
        }
    } else {
        data[line_index].erase(line_offset - bytes, bytes);
    }
}
