#pragma once

// #include "base/buffer/piece_table.h"
// #include "font/font_rasterizer.h"
#include "font/types.h"
#include "gui/renderer/types.h"
#include <vector>

namespace gui {

class LineLayoutCache {
public:
    const font::LineLayout& getLineLayout(std::string_view str8);
    int maxWidth() const;

private:
    // TODO: Clean this up.
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

    // TODO: Clean this up.
    // std::unordered_map<std::string, font::LineLayout> cache;
    std::unordered_map<std::string, font::LineLayout, transparent_string_hash, std::equal_to<>>
        cache;

    // TODO: Use a data structure (priority queue) for efficient updating.
    int max_width = 0;
};

}
