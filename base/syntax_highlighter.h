#pragma once

#include "base/rgb.h"
#include "config/color_scheme.h"
#include <string>
#include <tree_sitter/api.h>
#include <vector>

class SyntaxHighlighter {
public:
    SyntaxHighlighter();
    ~SyntaxHighlighter();
    void setLanguage(std::string scope, config::ColorScheme& color_scheme);
    void parse(TSInput& input);
    void edit(size_t start_byte, size_t old_end_byte, size_t new_end_byte);
    void getHighlights(TSPoint start_point, TSPoint end_point);
    Rgb getColor(size_t byte_offset, config::ColorScheme& color_scheme);

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
