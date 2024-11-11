#pragma once

#include "base/buffer/piece_tree.h"
#include <cstring>
#include <format>
#include <string>
#include <tree_sitter/api.h>
#include <vector>

inline constexpr bool operator==(const TSPoint& p1, const TSPoint& p2) {
    return p1.row == p2.row && p1.column == p2.column;
}
inline constexpr auto operator<=>(const TSPoint& p1, const TSPoint& p2) {
    return std::tie(p1.row, p1.column) <=> std::tie(p2.row, p2.column);
}

namespace base {

class SyntaxHighlighter {
public:
    SyntaxHighlighter();
    ~SyntaxHighlighter();

    void setCppLanguage();
    void parse(const PieceTree& piece_tree);
    void edit(size_t start_byte, size_t old_end_byte, size_t new_end_byte);

    struct Highlight {
        TSPoint start;
        TSPoint end;
        size_t capture_index;

        bool containsPoint(const TSPoint& p) const {
            return start <= p && p < end;
        }

        friend constexpr bool operator==(const Highlight& h1, const Highlight& h2) {
            return h1.start == h2.start && h1.end == h2.end;
        }

        friend constexpr auto operator<=>(const Highlight& h1, const Highlight& h2) {
            return std::tie(h1.start, h1.end) <=> std::tie(h2.start, h2.end);
        }
    };
    struct Rgb {
        uint8_t r, g, b;
    };
    std::vector<Highlight> getHighlights(size_t start_line, size_t end_line) const;
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

}  // namespace base

template <>
struct std::formatter<TSPoint> {
    constexpr auto parse(auto& ctx) {
        return ctx.begin();
    }

    auto format(const auto& p, auto& ctx) const {
        return std::format_to(ctx.out(), "TSPoint({}, {})", p.row, p.column);
    }
};

template <>
struct std::formatter<base::SyntaxHighlighter::Rgb> {
    constexpr auto parse(auto& ctx) {
        return ctx.begin();
    }

    auto format(const auto& rgb, auto& ctx) const {
        return std::format_to(ctx.out(), "Rgb({}, {}, {})", rgb.r, rgb.g, rgb.b);
    }
};
