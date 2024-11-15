#pragma once

#include "base/buffer/piece_tree.h"
#include "syntax_highlighter/language.h"
#include "syntax_highlighter/types.h"

namespace highlight {

class ParseTree {
public:
    ParseTree() = default;
    ~ParseTree();
    ParseTree(ParseTree&& other);
    ParseTree& operator=(ParseTree&& other);
    ParseTree(const ParseTree&) = delete;
    void operator=(const ParseTree&) = delete;

    void parse(const base::PieceTree& piece_tree, const Language& language);
    void edit(size_t start_byte, size_t old_end_byte, size_t new_end_byte);
    TSTree* getTree() const;

private:
    TSTree* tree = nullptr;
};

}  // namespace highlight
