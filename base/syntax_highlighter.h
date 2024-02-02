#include "third_party/tree_sitter/include/tree_sitter/api.h"

class SyntaxHighlighter {
public:
    SyntaxHighlighter();
    void edit(size_t start_byte, size_t old_end_byte, size_t new_end_byte);

    // private:
    TSParser* parser;
    TSQuery* query;
    TSTree* tree = NULL;
};
