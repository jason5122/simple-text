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

    BufferType::iterator begin() {
        return data.begin();
    }

    BufferType::iterator end() {
        return data.end();
    }

    // private:
    BufferType data;
};
