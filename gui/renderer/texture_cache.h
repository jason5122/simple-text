#pragma once

#include "app/types.h"
#include "font/font_rasterizer.h"
#include "gui/renderer/atlas.h"
#include "gui/renderer/types.h"

#include "third_party/hash_maps/robin_hood.h"

#include <vector>

namespace gui {

// TODO: Consider moving glyph/image loading code outside of this.
class TextureCache : util::NonCopyable {
public:
    TextureCache();

    struct Glyph {
        int32_t left;
        int32_t top;
        int32_t width;
        int32_t height;
        Vec4 uv;
        bool colored;
        size_t page;
    };
    const Glyph& getGlyph(size_t font_id, uint32_t glyph_id);

    struct Image {
        app::Size size;
        Vec4 uv;
        size_t page;
    };
    size_t addPng(std::string_view image_path);
    size_t addJpeg(std::string_view image_path);
    const Image& getImage(size_t image_id) const;

    constexpr const std::vector<Atlas>& pages() const;
    constexpr size_t pageCount() const;

private:
    std::vector<Atlas> atlas_pages;
    size_t current_page = 0;

    // We use a node-based map since we need to keep references stable.
    std::vector<robin_hood::unordered_node_map<uint32_t, Glyph>> cache;
    std::vector<Image> image_cache;

    Glyph insertIntoAtlas(const font::RasterizedGlyph& rglyph);
    bool loadPng(std::string_view file_name, Image& image);
    bool loadJpeg(std::string_view file_name, Image& image);
};

constexpr const std::vector<Atlas>& TextureCache::pages() const {
    return atlas_pages;
}

constexpr size_t TextureCache::pageCount() const {
    return atlas_pages.size();
}

}  // namespace gui
