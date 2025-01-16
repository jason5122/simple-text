#include "texture_cache.h"

#include <jerror.h>
#include <jpeglib.h>
#include <spng.h>

#include "base/files/file_util.h"
#include "base/files/scoped_file.h"

// TODO: Debug use; remove this.
#include <fmt/base.h>

namespace gui {

static_assert(!std::is_copy_constructible_v<TextureCache>);
static_assert(!std::is_copy_assignable_v<TextureCache>);
static_assert(std::is_move_constructible_v<TextureCache>);
static_assert(std::is_move_assignable_v<TextureCache>);

TextureCache::TextureCache() {
    atlas_pages.emplace_back();
}

const TextureCache::Glyph& TextureCache::getGlyph(size_t font_id, uint32_t glyph_id) {
    if (cache.size() <= font_id) {
        cache.resize(font_id + 1);
    }

    if (!cache[font_id].contains(glyph_id)) {
        const auto& font_rasterizer = font::FontRasterizer::instance();
        auto rglyph = font_rasterizer.rasterize(font_id, glyph_id);
        cache[font_id].emplace(glyph_id, insertIntoAtlas(std::move(rglyph)));
    }
    return cache[font_id][glyph_id];
}

// TODO: De-duplicate this code in a clean way.
size_t TextureCache::addPng(const base::FilePath& path) {
    Image image;
    bool success = loadPng(path, image);
    // TODO: Handle image load failure in a more robust way.
    if (!success) {
        fmt::println("TextureCache::addPng() error: Could not load image.");
        std::abort();
    }

    size_t image_id = image_cache.size();
    image_cache.emplace_back(std::move(image));
    return image_id;
}

// TODO: De-duplicate this code in a clean way.
size_t TextureCache::addJpeg(const base::FilePath& path) {
    Image image;
    bool success = loadJpeg(path, image);
    // TODO: Handle image load failure in a more robust way.
    if (!success) {
        fmt::println("TextureCache::addJpeg() error: Could not load image.");
    }

    size_t image_id = image_cache.size();
    image_cache.emplace_back(std::move(image));
    return image_id;
}

const TextureCache::Image& TextureCache::getImage(size_t image_id) const {
    return image_cache[image_id];
}

// TODO: Refactor recursion.
TextureCache::Glyph TextureCache::insertIntoAtlas(const font::RasterizedGlyph& rglyph) {
    Atlas& atlas = atlas_pages[current_page];

    // TODO: Handle the case when a texture is too large for the atlas.
    //       Return an enum classifying the error instead of using a boolean.
    Vec4 uv;
    bool success =
        atlas.insertTexture(rglyph.width, rglyph.height, Atlas::Format::kBGRA, rglyph.buffer, uv);

    // The current page is full, so create a new page and try again.
    if (!success) {
        atlas_pages.emplace_back();
        ++current_page;
        return insertIntoAtlas(rglyph);
    }

    return {
        .left = rglyph.left,
        .top = rglyph.top,
        .width = rglyph.width,
        .height = rglyph.height,
        .uv = uv,
        .colored = rglyph.colored,
        .page = current_page,
    };
}

// TODO: Handle errors.
bool TextureCache::loadPng(const base::FilePath& path, Image& image) {
    base::ScopedFILE fp(base::OpenFile(path, "rb"));
    std::unique_ptr<spng_ctx, void (*)(spng_ctx*)> ctx{spng_ctx_new(0), spng_ctx_free};
    spng_ihdr ihdr;
    size_t size;

    if (!fp) return false;
    if (!ctx) return false;

    if (spng_set_png_file(ctx.get(), fp.get())) return false;

    if (spng_get_ihdr(ctx.get(), &ihdr)) return false;

    if (spng_decoded_image_size(ctx.get(), SPNG_FMT_RGBA8, &size)) return false;

    std::vector<uint8_t> buffer(size);
    if (spng_decode_image(ctx.get(), buffer.data(), size, SPNG_FMT_RGBA8, SPNG_DECODE_TRNS)) {
        return false;
    }

    uint32_t width = ihdr.width;
    uint32_t height = ihdr.height;

    // TODO: Handle case when atlas is full.
    Atlas& atlas = atlas_pages[current_page];
    Vec4 uv;
    atlas.insertTexture(width, height, Atlas::Format::kRGBA, std::move(buffer), uv);
    image = {
        .size = {static_cast<int>(width), static_cast<int>(height)},
        .uv = uv,
        .page = current_page,
    };
    return true;
}

// TODO: Handle errors.
bool TextureCache::loadJpeg(const base::FilePath& path, Image& image) {
    jpeg_decompress_struct info;
    jpeg_error_mgr err;

    base::ScopedFILE fp(base::OpenFile(path, "rb"));
    if (!fp) {
        return false;
    }

    info.err = jpeg_std_error(&err);
    jpeg_create_decompress(&info);

    jpeg_stdio_src(&info, fp.get());
    jpeg_read_header(&info, TRUE);

    jpeg_start_decompress(&info);
    JDIMENSION width = info.output_width;
    JDIMENSION height = info.output_height;
    int num_components = info.num_components;
    size_t row_bytes = width * num_components;

    std::vector<uint8_t> buffer(width * height * num_components);
    uint8_t* row_buffer[1];
    while (info.output_scanline < height) {
        row_buffer[0] = &buffer[info.output_scanline * row_bytes];
        jpeg_read_scanlines(&info, row_buffer, 1);
    }

    jpeg_finish_decompress(&info);
    jpeg_destroy_decompress(&info);

    // TODO: Handle case when atlas is full.
    Atlas& atlas = atlas_pages[current_page];
    Vec4 uv;
    atlas.insertTexture(width, height, Atlas::Format::kRGB, std::move(buffer), uv);
    image = {
        .size = {static_cast<int>(width), static_cast<int>(height)},
        .uv = uv,
        .page = current_page,
    };
    return true;
}

}  // namespace gui
