#include "canvas/canvas.h"
#include "canvas/glyph_atlas.h"
#include "text/font_rasterizer.h"

namespace canvas {

Canvas::Canvas(gfx::Device& device) : device_(device) {}
Canvas::~Canvas() = default;

GlyphAtlas& Canvas::atlas() {
    if (!atlas_) atlas_ = std::make_unique<GlyphAtlas>(device_);
    return *atlas_;
}

Image* Canvas::create_image(int width,
                            int height,
                            gfx::TextureFormat format,
                            std::span<const uint8_t> pixels) {
    // Allocate an empty page, then upload via update_region — the same path the glyph atlas uses.
    auto texture = device_.create_texture(width, height, format);
    texture->update_region(0, 0, width, height, format, pixels);
    images_.push_back(std::make_unique<Image>(std::move(texture)));
    return images_.back().get();
}

void Canvas::fill_rect(const Rect& rect, const Color& color) {
    commands_.push_back({.kind = Command::Kind::kSolid, .dst = rect, .color = color});
}

void Canvas::draw_image(Image& image, const Rect& dst) {
    commands_.push_back({.kind = Command::Kind::kTextured,
                         .dst = dst,
                         .color = {1.0f, 1.0f, 1.0f, 1.0f},
                         .texture = &image.texture(),
                         .blend = gfx::BlendMode::kAlpha});
}

void Canvas::draw_text(const text::LineLayout& layout, text::Point origin, const Color& color) {
    auto& rasterizer = text::FontRasterizer::instance();
    const int baseline_y = origin.y + rasterizer.metrics(layout.layout_font_id).ascent;

    for (const text::ShapedGlyph& g : layout.glyphs) {
        const GlyphEntry& entry = atlas().get(g.font_id, g.glyph_id);
        if (entry.empty) continue;

        const float x = static_cast<float>(origin.x + g.position.x + entry.left);
        const float y = static_cast<float>(baseline_y + g.position.y - entry.top);

        // Color glyphs (emoji) carry their own color; tint with white to preserve it.
        const Color tint = entry.colored ? Color{1.0f, 1.0f, 1.0f, 1.0f} : color;

        commands_.push_back({.kind = Command::Kind::kTextured,
                             .dst = {x, y, static_cast<float>(entry.width),
                                     static_cast<float>(entry.height)},
                             .color = tint,
                             .texture = &atlas().page(entry.page),
                             .blend = gfx::BlendMode::kPremultiplied,
                             .u0 = entry.u0,
                             .v0 = entry.v0,
                             .u1 = entry.u1,
                             .v1 = entry.v1});
    }
}

void Canvas::flush(gfx::Frame& frame) {
    std::vector<gfx::Quad> solid_batch;
    std::vector<gfx::TexturedQuad> tex_batch;
    gfx::Texture* tex_batch_texture = nullptr;
    gfx::BlendMode tex_batch_blend = gfx::BlendMode::kAlpha;

    auto flush_solid = [&] {
        if (solid_batch.empty()) return;
        frame.draw_quads(solid_batch, 0, 0);
        solid_batch.clear();
    };
    auto flush_tex = [&] {
        if (tex_batch.empty()) return;
        frame.draw_textured_quads(tex_batch, *tex_batch_texture, tex_batch_blend, 0, 0);
        tex_batch.clear();
        tex_batch_texture = nullptr;
    };

    for (const Command& cmd : commands_) {
        if (cmd.kind == Command::Kind::kSolid) {
            flush_tex();
            solid_batch.push_back({cmd.dst.x, cmd.dst.y, cmd.dst.w, cmd.dst.h, cmd.color.r,
                                   cmd.color.g, cmd.color.b, cmd.color.a});
        } else {
            flush_solid();
            // Break the batch when the texture page or blend mode changes.
            if (tex_batch_texture &&
                (tex_batch_texture != cmd.texture || tex_batch_blend != cmd.blend)) {
                flush_tex();
            }
            tex_batch_texture = cmd.texture;
            tex_batch_blend = cmd.blend;
            tex_batch.push_back({cmd.dst.x, cmd.dst.y, cmd.dst.w, cmd.dst.h, cmd.u0, cmd.v0, cmd.u1,
                                 cmd.v1, cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a});
        }
    }
    flush_solid();
    flush_tex();

    commands_.clear();
}

}  // namespace canvas
