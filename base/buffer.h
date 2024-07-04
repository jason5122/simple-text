#pragma once

#include <cstring>
#include <string>
#include <vector>

namespace base {

class Buffer {
public:
    void setContents(const std::string& text);
    size_t size() const;
    size_t lineCount() const;
    std::string getLineContent(size_t line_index) const;
    const std::vector<int>& getUtf8Offsets(size_t line_index) const;
    size_t byteOfLine(size_t line_index) const;
    void insert(size_t line_index, size_t line_offset, std::string_view text);
    void remove(size_t line_index, size_t line_offset, size_t bytes);
    void backspace(size_t line_index, size_t line_offset, size_t bytes);
    void erase(size_t start_byte, size_t end_byte);
    bool empty();

private:
    std::vector<std::string> data;
    std::string flat_string;
    std::vector<std::vector<int>> utf8_offsets;
};

}
