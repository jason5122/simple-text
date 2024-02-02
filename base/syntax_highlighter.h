#include "third_party/tree_sitter/include/tree_sitter/api.h"

class SyntaxHighlighter {
public:
    SyntaxHighlighter();

    // private:
    TSParser* parser;
    TSQuery* query;
};
