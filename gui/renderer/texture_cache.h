#pragma once

#include "base/files/file_path.h"
#include "font/font_rasterizer.h"
#include "gui/renderer/atlas.h"
#include "gui/renderer/types.h"
#include "gui/types.h"

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
    const Glyph& get_glyph(size_t font_id, uint32_t glyph_id);

    struct Image {
        Size size;
        Vec4 uv;
        size_t page;
    };
    size_t add_png(const base::FilePath& path);
    size_t add_jpeg(const base::FilePath& path);
    const Image& get_image(size_t image_id) const;

    constexpr const std::vector<Atlas>& pages() const { return atlas_pages; }

private:
    std::vector<Atlas> atlas_pages;
    size_t current_page = 0;

    // We use a node-based map since we need to keep references stable.
    std::vector<robin_hood::unordered_node_map<uint32_t, Glyph>> cache;
    std::vector<Image> image_cache;

    Glyph insert_into_atlas(const font::RasterizedGlyph& rglyph);
    bool load_png(const base::FilePath& path, Image& image);
    bool load_jpeg(const base::FilePath& path, Image& image);
};

}  // namespace gui
