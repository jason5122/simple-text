#pragma once

#include "base/files/file_path.h"
#include "fx/fx.h"
#include "gui/renderer/atlas.h"
#include "gui/renderer/types.h"
#include "gui/types.h"
#include "third_party/hash_maps/robin_hood.h"
#include <vector>

namespace gui {

// TODO: Consider moving glyph/image loading code outside of this.
class TextureCache {
public:
    TextureCache();

    struct Glyph {
        // Device-pixel offset from the glyph's pen position on the alphabetic baseline to the
        // top-left corner of its bitmap, y-down. `bearing_y` is therefore usually negative.
        int32_t bearing_x;
        int32_t bearing_y;
        int32_t width;
        int32_t height;
        Vec4 uv;
        bool colored;
        size_t page;
    };
    // `font_id` is a FontCache ID and `glyph_id` an fx glyph ID from a layout in that font. A
    // glyph with no ink (e.g. a space) has zero width and height.
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

    Glyph insert_into_atlas(const fx_glyph_cache::glyph_phase& phase, bool colored);
    bool load_png(const base::FilePath& path, Image& image);
    bool load_jpeg(const base::FilePath& path, Image& image);
};

static_assert(std::movable<TextureCache>);
static_assert(std::copyable<TextureCache>);

}  // namespace gui
