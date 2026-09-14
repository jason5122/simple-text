#pragma once

#include "editor/line_layout.h"
#include "gui/renderer/shader.h"
#include "gui/renderer/types.h"
#include "gui/types.h"
#include <functional>
#include <vector>

namespace gui {

class TextureRenderer {
public:
    TextureRenderer();
    ~TextureRenderer();
    TextureRenderer(const TextureRenderer&) = delete;
    TextureRenderer& operator=(const TextureRenderer&) = delete;
    TextureRenderer(TextureRenderer&& other) noexcept;
    TextureRenderer& operator=(TextureRenderer&& other) noexcept;

    // Draws `line_layout` with the top-left corner of its line box at `coords`; the text's
    // alphabetic baseline sits at `coords.y + ascent`. Only the part of each glyph inside the
    // rectangle spanning [min_coords, max_coords) is drawn. All coordinates are absolute device
    // pixels with y growing downward. `highlight_callback` maps a glyph's UTF-8 index to its
    // color.
    void add_line_layout(const editor::LineLayout& line_layout,
                         const Point& coords,
                         const Point& min_coords,
                         const Point& max_coords,
                         const std::function<Rgb(size_t)>& highlight_callback);
    void add_image(size_t image_index, const Point& coords, const Rgb& color);
    void add_color_image(size_t image_index, const Point& coords);
    void flush(const Size& screen_size);

private:
    static constexpr size_t kBatchMax = 0x10000;

    Shader shader_program;
    gl::GLuint vao = 0;
    gl::GLuint vbo_instance = 0;
    gl::GLuint ebo = 0;

    struct InstanceData {
        Vec2 coords;
        Vec4 glyph;
        Vec4 uv;
        Rgba color;
    };

    std::vector<std::vector<InstanceData>> batches;

    void insert_into_batch(size_t page, const InstanceData& instance);

    // Packed into the alpha channel of InstanceData::color. Keep in sync with texture_frag.glsl.
    enum InstanceKind {
        // Alpha mask tinted with the instance color (icons).
        kPlainTexture = 0,
        // Premultiplied color bitmap (emoji).
        kColoredText = 1,
        // Straight-alpha RGBA image drawn as-is.
        kColoredImage = 2,
        // Per-channel coverage in RGB tinted with the instance color (fx monochrome glyphs).
        kMonochromeText = 3,
    };

    // DEBUG: Draws texture atlases.
    friend class AtlasWidget;
    void render_atlas_page(size_t page,
                           const Point& coords,
                           const Point& min_coords,
                           const Point& max_coords);
};

static_assert(std::movable<TextureRenderer>);
static_assert(!std::copyable<TextureRenderer>);

}  // namespace gui
