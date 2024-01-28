#pragma once

#include <fstream>
#include <string>
#include <vector>

class Buffer {
public:
    using BufferType = std::vector<std::string>;

    Buffer(std::string s);
    Buffer(std::ifstream& istrm);
    size_t lineCount();
    std::string line(size_t line_index);
    size_t byteOfLine(size_t line_index);

    BufferType::iterator begin() {
        return data.begin();
    }

    BufferType::iterator end() {
        return data.end();
    }

    // private:
    BufferType data;
};
