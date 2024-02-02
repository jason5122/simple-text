#include "base/rgb.h"
#include "third_party/tree_sitter/include/tree_sitter/api.h"
#include <vector>

class SyntaxHighlighter {
public:
    SyntaxHighlighter();
    void parse(TSInput& input);
    void edit(size_t start_byte, size_t old_end_byte, size_t new_end_byte);
    void getHighlights();

    // private:
    TSParser* parser;
    TSQuery* query;
    TSTree* tree = NULL;

    std::vector<std::pair<uint32_t, uint32_t>> highlight_ranges;
    std::vector<Rgb> highlight_colors;
};
