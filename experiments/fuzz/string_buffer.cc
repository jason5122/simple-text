#include "experiments/fuzz/string_buffer.h"
#include <string_view>
#include <vector>

std::string StringBuffer::get_line_content(size_t line) const {
    size_t curr_line = 0;
    size_t i = 0;
    while (i < buffer.length() && curr_line < line) {
        if (buffer[i++] == '\n') curr_line++;
    }
    std::string content;
    while (i < buffer.length() && buffer[i] != '\n') {
        content += buffer[i++];
    }
    return content;
}

std::string StringBuffer::get_line_content_with_newline(size_t line) const {
    size_t curr_line = 0;
    size_t i = 0;
    while (i < buffer.length() && curr_line < line) {
        if (buffer[i++] == '\n') curr_line++;
    }
    std::string content;
    while (i < buffer.length() && buffer[i] != '\n') {
        content += buffer[i++];
    }
    if (i < buffer.length()) content += buffer[i];  // Add newline.
    return content;
}

size_t StringBuffer::line_at(size_t offset) const {
    offset = std::min(offset, buffer.length());
    size_t line = 0;
    for (size_t i = 0; i < offset; i++) {
        if (buffer[i] == '\n') line++;
    }
    return line;
}

namespace {
std::vector<size_t> line_starts(std::string_view s) {
    std::vector<size_t> starts;
    starts.push_back(0);
    for (size_t i = 0; i < s.length(); i++) {
        if (s[i] == '\n') starts.push_back(i + 1);
    }
    return starts;
}
}  // namespace

size_t StringBuffer::offset_at(size_t line, size_t column) const {
    auto starts = line_starts(buffer);
    if (starts.empty()) return 0;
    if (line >= starts.size()) line = starts.size() - 1;

    size_t line_start = starts[line];
    // line end is either next start - 1, or end
    size_t line_end = (line + 1 < starts.size()) ? (starts[line + 1] - 1) : buffer.length();
    size_t line_len = (line_end >= line_start) ? (line_end - line_start) : 0;
    column = std::min(column, line_len);
    return line_start + column;
}
