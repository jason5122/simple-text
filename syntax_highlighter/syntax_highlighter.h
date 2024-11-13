#pragma once

#include "base/buffer/piece_tree.h"
#include "syntax_highlighter/types.h"
#include <cstring>
#include <format>
#include <string>
#include <vector>

namespace highlight {

class QueryCursor {
public:
    ~QueryCursor();

    bool nextMatch(TSPoint& start, TSPoint& end, uint32_t& capture_index);

private:
    friend class SyntaxHighlighter;

    QueryCursor(size_t start_line, size_t end_line, TSTree* tree, TSQuery* query);

    TSQueryCursor* cursor;
};

class SyntaxHighlighter {
public:
    SyntaxHighlighter();
    ~SyntaxHighlighter();

    void setCppLanguage();
    void parse(const base::PieceTree& piece_tree);
    void edit(size_t start_byte, size_t old_end_byte, size_t new_end_byte);

    std::vector<Highlight> getHighlights(size_t start_line, size_t end_line) const;
    QueryCursor startQuery(size_t start_line, size_t end_line) const;
    const Rgb& getColor(size_t capture_index) const;

private:
    TSParser* parser = nullptr;
    TSQuery* query = nullptr;
    TSTree* tree = nullptr;

    // TODO: Support multiple languages.
    const TSLanguage* json_language = nullptr;
    wasm_engine_t* engine = nullptr;
    TSWasmStore* wasm_store = nullptr;

    std::vector<Rgb> capture_index_color_table;

    void loadFromWasm();
};

}  // namespace highlight
