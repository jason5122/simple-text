#pragma once

#include "base/buffer/piece_table.h"
#include "font/font_rasterizer.h"
#include "gui/renderer/types.h"
#include <vector>

namespace gui {

class LineLayoutCache {
public:
    LineLayoutCache(const base::PieceTable& table);

    const font::LineLayout& getLineLayout(size_t line) const;
    const font::LineLayout& getLineLayout(std::string_view str8);
    void reflow(const base::PieceTable& table, size_t line);
    int maxWidth() const;

    struct Caret;
    void moveToPoint(size_t line, const Point& point, Caret& caret) const;
    void moveByCharacters(bool forward, Caret& caret) const;

private:
    // https://www.reddit.com/r/cpp_questions/comments/12xw3sn/comment/jhki225/?utm_source=share&utm_medium=web3x&utm_name=web3xcss&utm_term=1&utm_content=share_button
    template <typename... Bases> struct overload : Bases... {
        using is_transparent = void;
        using Bases::operator()...;
    };
    struct char_pointer_hash {
        auto operator()(const char* ptr) const noexcept {
            return std::hash<std::string_view>{}(ptr);
        }
    };
    using transparent_string_hash =
        overload<std::hash<std::string>, std::hash<std::string_view>, char_pointer_hash>;

    std::vector<font::LineLayout> line_layouts;
    // std::unordered_map<std::string, font::LineLayout> cache;
    std::unordered_map<std::string, font::LineLayout, transparent_string_hash, std::equal_to<>>
        cache;

    // TODO: Use a data structure (priority queue) for efficient updating.
    int max_width = 0;
};

struct LineLayoutCache::Caret {
    size_t line;
    size_t index;  // UTF-8 index in line.
    int x;
    // We use this value to position the caret during vertical movement.
    // This is updated whenever the caret moves horizontally.
    int max_x;

    // TODO: Is this the best implementation?
    size_t run_index;
    size_t run_glyph_index;

    friend constexpr bool operator<(const Caret& c1, const Caret& c2) {
        if (c1.line == c2.line) {
            return c1.index < c2.index;
        } else {
            return c1.line < c2.line;
        }
    }
    friend constexpr bool operator>(const Caret& c1, const Caret& c2) {
        return operator<(c2, c1);
    }
};

static_assert(LineLayoutCache::Caret{0, 0} < LineLayoutCache::Caret{0, 1});
static_assert(LineLayoutCache::Caret{0, 1} < LineLayoutCache::Caret{1, 0});
static_assert(LineLayoutCache::Caret{1, 0} < LineLayoutCache::Caret{1, 1});
static_assert(!(LineLayoutCache::Caret{1, 0} < LineLayoutCache::Caret{1, 0}));

static_assert(LineLayoutCache::Caret{0, 1} > LineLayoutCache::Caret{0, 0});
static_assert(LineLayoutCache::Caret{1, 0} > LineLayoutCache::Caret{0, 1});
static_assert(LineLayoutCache::Caret{1, 1} > LineLayoutCache::Caret{1, 0});
static_assert(!(LineLayoutCache::Caret{1, 0} > LineLayoutCache::Caret{1, 0}));

}
