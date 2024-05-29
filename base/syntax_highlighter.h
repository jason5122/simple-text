#pragma once

#include "base/buffer.h"
#include "base/rgb.h"
#include "config/color_scheme.h"
#include "util/not_copyable_or_movable.h"
#include <string>
#include <tree_sitter/api.h>
#include <vector>

namespace base {

class SyntaxHighlighter {
public:
    NOT_COPYABLE(SyntaxHighlighter)
    NOT_MOVABLE(SyntaxHighlighter)
    SyntaxHighlighter();
    ~SyntaxHighlighter();

    void setLanguage(std::string scope, config::ColorScheme& color_scheme);
    void parse(TSInput& input);
    void edit(size_t start_byte, size_t old_end_byte, size_t new_end_byte);
    void getHighlights(TSPoint start_point, TSPoint end_point);
    Rgb getColor(size_t byte_offset, config::ColorScheme& color_scheme);

    static const char* read(void* payload, uint32_t byte_index, TSPoint position,
                            uint32_t* bytes_read) {
        Buffer* buffer = (Buffer*)payload;
        if (position.row >= buffer->lineCount()) {
            *bytes_read = 0;
            return "";
        }

        const size_t BUFSIZE = 256;
        static char buf[BUFSIZE];

        std::string line_str = buffer->getLineContent(position.row);

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
    TSParser* parser = nullptr;
    TSQuery* query = nullptr;
    TSTree* tree = nullptr;

    std::string scope;

    size_t idx = 0;
    std::vector<size_t> capture_indexes;
    std::vector<Rgb> capture_index_color_table;
    std::vector<std::pair<uint32_t, uint32_t>> highlight_ranges;
};

}
