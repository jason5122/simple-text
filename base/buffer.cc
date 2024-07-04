#include "buffer.h"
#include <cstdint>

extern "C" {
#include "third_party/libgrapheme/grapheme.h"
}

// TODO: Debug use; remove this.
#include "util/profile_util.h"

namespace base {

void Buffer::setContents(const std::string& text) {
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

    {
        PROFILE_BLOCK("Buffer: collect UTF-8 chars");
        utf8_chars.resize(data.size());

        size_t byte_offset = 0;
        for (size_t i = 0; i < data.size(); i++) {
            const auto& line_str = data.at(i);

            size_t offset;
            for (size_t line_offset = 0; line_offset < line_str.size(); line_offset += offset) {
                offset = grapheme_next_character_break_utf8(&line_str[0] + line_offset, SIZE_MAX);
                utf8_chars.at(i).emplace_back(Utf8Char{
                    .str = std::string_view(line_str).substr(line_offset, offset),
                    .size = offset,
                    .line_offset = line_offset,
                    .byte_offset = byte_offset,
                });
                byte_offset += offset;
            }

            // Include newline.
            std::string_view newline = "\n";
            offset = newline.length();
            utf8_chars.at(i).emplace_back(Utf8Char{
                .str = newline,
                .size = offset,
                .line_offset = line_str.size(),
                .byte_offset = byte_offset,
            });
            byte_offset += offset;
        }
    }
}

size_t Buffer::size() const {
    size_t byte_count = 0;
    for (const std::string& line : data) {
        byte_count += line.size();
        byte_count += 1;  // Account for \n characters.
    }
    return byte_count;
}

size_t Buffer::lineCount() const {
    return data.size();
}

size_t Buffer::lineLength(size_t line_index) const {
    return data.at(line_index).length();
}

std::string Buffer::getLineContent(size_t line_index) const {
    return data.at(line_index);
}

const std::vector<Buffer::Utf8Char>& Buffer::getLineChars(size_t line_index) const {
    return utf8_chars.at(line_index);
}

size_t Buffer::byteOfLine(size_t line_index) const {
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

bool Buffer::empty() {
    return data.size() == 1 && data[0].empty();
}

}
