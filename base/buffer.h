#pragma once

#include <cstring>
#include <string>
#include <vector>

namespace base {

class Buffer {
public:
    struct Utf8Char {
        std::string_view str;
        size_t size;
        size_t line_offset;
        size_t byte_offset;
        size_t line;
    };

    std::vector<Utf8Char>::const_iterator begin() const;
    std::vector<Utf8Char>::const_iterator end() const;
    std::vector<Utf8Char>::const_iterator line(size_t line) const;

    void setContents(const std::string& text);
    size_t size() const;
    size_t lineCount() const;
    size_t lineLength(size_t line_index) const;
    std::string getLineContent(size_t line_index) const;
    size_t byteOfLine(size_t line_index) const;
    void insert(size_t line_index, size_t line_offset, std::string_view text);
    void remove(size_t line_index, size_t line_offset, size_t bytes);
    void backspace(size_t line_index, size_t line_offset, size_t bytes);
    void erase(size_t start_byte, size_t end_byte);
    bool empty();

private:
    static constexpr std::string_view kNewlineString = "\n";

    std::vector<std::string> data;
    std::vector<Utf8Char> utf8_chars;
    std::vector<size_t> newline_offsets;
};

}
