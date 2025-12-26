#pragma once

#include <algorithm>
#include <cstdlib>
#include <string>

// TODO: Can we inherit from std::string instead? Should we? Our interface is already simple.
class StringBuffer {
public:
    StringBuffer() : StringBuffer(std::string_view{}) {}
    explicit StringBuffer(std::string_view txt) : buffer(txt) {}

    // Manipulation.
    void insert(size_t offset, std::string_view txt) { buffer.insert(offset, txt); }
    void erase(size_t offset, size_t count) { buffer.erase(offset, count); }
    void clear() { buffer.clear(); }

    // Metadata.
    size_t length() const { return buffer.length(); }
    bool empty() const { return buffer.empty(); }
    size_t line_feed_count() const { return std::ranges::count(buffer, '\n'); }
    size_t line_count() const { return line_feed_count() + 1; }

    // Queries.
    std::string get_line_content(size_t line) const;
    std::string get_line_content_with_newline(size_t line) const;
    size_t line_at(size_t offset) const;
    size_t offset_at(size_t line, size_t column) const;
    std::string str() const { return buffer; }
    std::string substr(size_t offset, size_t count) const { return buffer.substr(offset, count); }

private:
    std::string buffer;
};
