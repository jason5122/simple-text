#include "buffer.h"
#include <cstdint>

extern "C" {
#include "third_party/libgrapheme/grapheme.h"
}

// TODO: Debug use; remove this.
#include <format>
#include <iostream>

namespace base {

Buffer::Iterator Buffer::begin() const {
    return utf8_chars.begin();
}

Buffer::Iterator Buffer::end() const {
    return utf8_chars.end();
}

Buffer::Iterator Buffer::getLine(size_t line) const {
    if (line >= newline_offsets.size()) {
        return end();
    } else {
        return utf8_chars.begin() + newline_offsets.at(line);
    }
}

// This does not return the ending newline, if any.
std::string_view Buffer::getLineContents(size_t line) const {
    size_t s1 = newline_offsets[line] + 1;
    size_t s2 = newline_offsets[line + 1];
    return std::string_view(text).substr(s1, s2 - s1);
}

Buffer::Buffer(const std::string& text) : text{text} {
    newline_offsets.emplace_back(-1);
    for (size_t i = 0; i < text.length(); i++) {
        if (text[i] == '\n') {
            // Cache byte offsets of newlines.
            newline_offsets.emplace_back(i);
            line_count++;
        }
    }
    newline_offsets.emplace_back(text.length());
    line_count++;

    size_t byte_offset = 0;
    for (size_t line = 0; line < lineCount(); line++) {
        std::string_view line_str = getLineContents(line);

        size_t offset;
        for (size_t line_offset = 0; line_offset < line_str.size(); line_offset += offset) {
            offset = grapheme_next_character_break_utf8(&line_str[0] + line_offset, SIZE_MAX);
            utf8_chars.emplace_back(Utf8Char{
                .str = std::string_view(line_str).substr(line_offset, offset),
                .size = offset,
                .line_offset = line_offset,
                .byte_offset = byte_offset,
                .line = line,
            });
            byte_offset += offset;
        }

        // Include newline.
        offset = kNewlineString.length();
        utf8_chars.emplace_back(Utf8Char{
            .str = kNewlineString,
            .size = offset,
            .line_offset = line_str.size(),
            .byte_offset = byte_offset,
            .line = line,
        });
        byte_offset += offset;
    }
}

size_t Buffer::lineCount() const {
    return line_count;
}

void Buffer::insert(size_t line_index, size_t line_offset, std::string_view text) {
    std::cerr << "Buffer::insert(): unimplemented!\n";
}

void Buffer::erase(size_t start_byte, size_t end_byte) {
    std::cerr << "Buffer::erase(): unimplemented!\n";
}

}
