#pragma once

#include "third_party/fredbuf/fredbuf.h"
#include <string>
#include <tree_sitter/api.h>
using namespace PieceTree;

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
    void debugInfo();

    static const char* read(void* payload, uint32_t byte_index, TSPoint position,
                            uint32_t* bytes_read) {
        Buffer* buffer = (Buffer*)payload;
        if (position.row >= buffer->lineCount()) {
            *bytes_read = 0;
            return "";
        }

        const size_t BUFSIZE = 256;
        static char buf[BUFSIZE];

        std::string line_str;
        buffer->getLineContent(&line_str, position.row);

        size_t len = line_str.size();
        size_t bytes_copied = std::min(len - position.column, BUFSIZE);

        memcpy(buf, &line_str[0] + position.column, bytes_copied);
        *bytes_read = (uint32_t)bytes_copied;
        if (bytes_copied < BUFSIZE) {
            // Add the final \n.
            // If it didn't fit, read() will be called again on the same line with the column
            // advanced.
            buf[bytes_copied] = '\n';
            (*bytes_read)++;
        }
        return buf;
    }

private:
    Tree piece_tree;

    using BufferType = std::vector<std::string>;

    BufferType::iterator begin() {
        return data.begin();
    }

    BufferType::iterator end() {
        return data.end();
    }
    BufferType data;
};
