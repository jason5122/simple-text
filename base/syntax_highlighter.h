#include "base/rgb.h"
#include <string>
#include <tree_sitter/api.h>
#include <vector>

class SyntaxHighlighter {
public:
    size_t idx = 0;
    std::vector<Rgb> highlight_colors;

    SyntaxHighlighter() = default;
    void setLanguage(std::string scope);
    void parse(TSInput& input);
    void edit(size_t start_byte, size_t old_end_byte, size_t new_end_byte);
    void getHighlights();
    bool isByteOffsetInRange(size_t byte_offset);

private:
    TSParser* parser;
    TSQuery* query;
    TSTree* tree = NULL;

    std::string scope;

    std::vector<std::pair<uint32_t, uint32_t>> highlight_ranges;
};
