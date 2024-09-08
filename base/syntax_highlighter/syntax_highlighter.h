#pragma once

#include "base/buffer/piece_table.h"
#include <string>
#include <tree_sitter/api.h>
#include <vector>

// Debug use; remove this.
#include <format>
#include <iostream>

namespace base {

class SyntaxHighlighter {
public:
    SyntaxHighlighter();
    ~SyntaxHighlighter();

    struct Rgb {
        int r, g, b;

        friend std::ostream& operator<<(std::ostream& out, const Rgb& rgb) {
            return out << std::format("Rgb{{{}, {}, {}}}", rgb.r, rgb.g, rgb.b);
        }
    };

    void mystery();
    // void setLanguage(std::string scope, config::ColorScheme& color_scheme);
    void parse(TSInput& input);
    // void edit(size_t start_byte, size_t old_end_byte, size_t new_end_byte);
    void getHighlights(size_t start_byte, size_t end_byte);
    Rgb getColor(size_t byte_offset);

    static const char* read(void* payload,
                            uint32_t byte_index,
                            TSPoint position,
                            uint32_t* bytes_read) {
        PieceTable* table = (PieceTable*)payload;

        if (position.row >= table->lineCount()) {
            *bytes_read = 0;
            return "";
        }

        static constexpr size_t kBufferSize = 256;
        static char buf[kBufferSize];

        std::string line_str = table->line(position.row);
        size_t bytes_copied = std::min(line_str.length() - position.column, kBufferSize);

        memcpy(buf, line_str.data() + position.column, bytes_copied);
        *bytes_read = bytes_copied;
        return buf;
    }

private:
    TSParser* parser = nullptr;
    TSQuery* query = nullptr;
    TSTree* tree = nullptr;

    // std::string scope;

    size_t idx = 0;
    std::vector<size_t> capture_indexes;
    std::vector<Rgb> capture_index_color_table;
    std::vector<std::pair<uint32_t, uint32_t>> highlight_ranges;
};

}
