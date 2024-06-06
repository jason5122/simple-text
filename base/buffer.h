#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

class Buffer {
public:
    Buffer() = default;
    void setContents(std::string txt);
    size_t size();
    size_t lineCount();
    void getLineContent(std::string* buf, size_t line_index) const;
    size_t byteOfLine(size_t line_index);
    void insert(size_t line_index, size_t line_offset, std::string_view txt);
    void remove(size_t line_index, size_t line_offset, size_t bytes);
    void backspace(size_t line_index, size_t line_offset, size_t bytes);
    void debugInfo() {
        fprintf(stderr, "size = %zu, lineCount = %zu\n", size(), lineCount());
    }

private:
    using BufferType = std::vector<std::string>;

    BufferType::iterator begin() {
        return data.begin();
    }

    BufferType::iterator end() {
        return data.end();
    }
    BufferType data;
};
