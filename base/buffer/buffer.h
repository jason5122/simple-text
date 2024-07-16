#pragma once

#include <cstring>
#include <string>
#include <vector>

namespace base {

class Buffer {
public:
    Buffer(const std::string& text);

    struct Utf8Char {
        std::string_view str;
        size_t size;
        size_t line_offset;
        size_t byte_offset;
        size_t line;
    };

    using Iterator = std::vector<Utf8Char>::const_iterator;
    using StringIterator = std::string::const_iterator;

    Iterator begin() const;
    Iterator end() const;
    StringIterator stringBegin() const;
    StringIterator stringEnd() const;
    // Iterator getLine(size_t line) const;
    std::string_view getLineContents(size_t line) const;
    size_t lineCount() const;
    void insert(Buffer::StringIterator pos, std::string_view value);
    void erase(Buffer::StringIterator first, Buffer::StringIterator last);
    std::string_view str() const;

private:
    static constexpr std::string_view kNewlineString = "\n";

    std::string text;
    std::vector<Utf8Char> utf8_chars;
    std::vector<size_t> newline_offsets;
    size_t line_count = 0;

    // TODO: This is a reference implementation. Don't do this for the actual piece table.
    void reflow();
};

}
