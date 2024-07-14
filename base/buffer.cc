#include "buffer.h"
#include <cstdint>

extern "C" {
#include "third_party/libgrapheme/grapheme.h"
}

// TODO: Debug use; remove this.
#include "util/profile_util.h"

namespace base {

std::vector<Buffer::Utf8Char>::const_iterator Buffer::begin() const {
    return utf8_chars.begin();
}

std::vector<Buffer::Utf8Char>::const_iterator Buffer::end() const {
    return utf8_chars.end();
}

std::vector<Buffer::Utf8Char>::const_iterator Buffer::line(size_t line) const {
    if (line >= newline_offsets.size()) {
        return end();
    } else {
        return utf8_chars.begin() + newline_offsets.at(line);
    }
}

Buffer::Buffer(const std::string& text) {
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

    {
        PROFILE_BLOCK("Buffer: collect UTF-8 chars");

        size_t byte_offset = 0;
        for (size_t line = 0; line < data.size(); line++) {
            const auto& line_str = data.at(line);

            // Cache byte offsets of newlines.
            newline_offsets.emplace_back(utf8_chars.size());

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
}

size_t Buffer::lineCount() const {
    return data.size();
}

void Buffer::insert(size_t line_index, size_t line_offset, std::string_view text) {
    std::cerr << "Buffer::insert(): unimplemented!\n";
}

void Buffer::erase(size_t start_byte, size_t end_byte) {
    std::cerr << "Buffer::erase(): unimplemented!\n";
}

}
