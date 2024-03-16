#pragma once

#include "third_party/fredbuf/fredbuf.h"
#include <fstream>
#include <string>
#include <vector>
using namespace PieceTree;

class Buffer {
public:
    using BufferType = std::vector<std::string>;

    Buffer() = default;
    void setContents(std::string txt);
    size_t size();
    size_t lineCount();
    void getLineContent(std::string* buf, size_t line_index) const;
    size_t byteOfLine(size_t line_index);
    void insert(size_t line_index, size_t line_offset, std::string_view txt);
    void remove(size_t line_index, size_t line_offset, size_t bytes);

    BufferType::iterator begin() {
        return data.begin();
    }

    BufferType::iterator end() {
        return data.end();
    }

private:
    BufferType data;
    Tree piece_tree;
};
