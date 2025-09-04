#pragma once

#include "editor/buffer/piece_tree.h"
#include "types.h"
#include <tree_sitter/api.h>

namespace highlight {

class Language {
public:
    Language(wasm_engine_t* engine, std::string_view name);
    ~Language();
    Language(Language&& other);
    Language& operator=(Language&& other);
    Language(const Language&) = delete;
    void operator=(const Language&) = delete;

    void load();
    std::vector<Highlight> highlight(TSTree* tree, size_t start_line, size_t end_line) const;
    const Rgb& getColor(size_t capture_index) const;
    TSParser* getParser() const;

private:
    std::string name;
    TSParser* parser = nullptr;
    TSWasmStore* wasm_store = nullptr;

    const TSLanguage* language = nullptr;
    TSQuery* query = nullptr;
    std::vector<Rgb> capture_index_color_table;
};

}  // namespace highlight
