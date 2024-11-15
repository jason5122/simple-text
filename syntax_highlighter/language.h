#pragma once

#include "base/buffer/piece_tree.h"
#include "types.h"
#include <tree_sitter/api.h>

namespace highlight {

class Language {
public:
    Language();
    ~Language();

    void load(std::string_view language_name);
    std::vector<Highlight> highlight(TSTree* tree, size_t start_line, size_t end_line) const;
    const Rgb& getColor(size_t capture_index) const;
    TSParser* getParser() const;

private:
    wasm_engine_t* engine;  // TODO: Refactor this.

    TSParser* parser = nullptr;
    TSWasmStore* wasm_store = nullptr;

    const TSLanguage* language = nullptr;
    TSQuery* query = nullptr;
    std::vector<Rgb> capture_index_color_table;
};

}  // namespace highlight
