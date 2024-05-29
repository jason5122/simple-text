#pragma once

#include <cstring>
#include <string>
#include <vector>

namespace base {

class Buffer {
public:
    void setContents(std::string txt);
    size_t size();
    size_t lineCount();
    std::string getLineContent(size_t line_index) const;
    size_t byteOfLine(size_t line_index);
    void insert(size_t line_index, size_t line_offset, std::string_view txt);
    void remove(size_t line_index, size_t line_offset, size_t bytes);
    void backspace(size_t line_index, size_t line_offset, size_t bytes);
    void erase(size_t start_byte, size_t end_byte);

private:
    using BufferType = std::vector<std::string>;

    BufferType::iterator begin() {
        return data.begin();
    }

    BufferType::iterator end() {
        return data.end();
    }
    BufferType data;

    std::string flat_string;
};

}
