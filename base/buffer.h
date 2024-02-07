#pragma once

#include <fstream>
#include <string>
#include <vector>

class Buffer {
public:
    using BufferType = std::vector<std::string>;

    Buffer() = default;
    void setContents(std::string s);
    void setContents(std::ifstream& istrm);
    size_t lineCount();
    size_t byteCount();
    void getLineContent(std::string* buf, size_t line_index) const;
    size_t byteOfLine(size_t line_index);
    void insert(size_t line_index, size_t line_offset, std::string_view txt);

    BufferType::iterator begin() {
        return data.begin();
    }

    BufferType::iterator end() {
        return data.end();
    }

    // private:
    BufferType data;
};
