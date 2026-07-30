#pragma once

#include "gfx/device.h"
#include "gfx/frame.h"
#include "gfx/texture.h"
#include "text/types.h"
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace canvas {

class GlyphAtlas;

struct Rect {
    float x, y, w, h;
};

struct Color {
    float r, g, b, a;
};

// An image registered with a Canvas, backed by a single GPU texture.
class Image {
public:
    explicit Image(std::unique_ptr<gfx::Texture> texture) : texture_(std::move(texture)) {}

    int width() const { return texture_->width(); }
    int height() const { return texture_->height(); }
    gfx::Texture& texture() { return *texture_; }

private:
    std::unique_ptr<gfx::Texture> texture_;
};

// A 2D painter over //gfx. Records fill_rect / draw_image / draw_text in submission order and, on
// flush(), emits batched //gfx draws preserving that order — so later draws paint over earlier ones
// by construction. Owns the textures backing images and the glyph atlas.
class Canvas {
public:
    explicit Canvas(gfx::Device& device);
    ~Canvas();

    // Registers an image from tightly-packed `pixels` and returns a non-owning handle (owned by
    // this Canvas). Valid until the Canvas is destroyed.
    Image* create_image(int width,
                        int height,
                        gfx::TextureFormat format,
                        std::span<const uint8_t> pixels);

    void fill_rect(const Rect& rect, const Color& color);
    void draw_image(Image& image, const Rect& dst);
    // Draws a shaped line. `origin` is the top-left of the line box (in device pixels); the baseline
    // is derived from the layout font's ascent.
    void draw_text(const text::LineLayout& layout, text::Point origin, const Color& color);

    // Emits all recorded draws into `frame` in submission order, then clears the draw list.
    void flush(gfx::Frame& frame);

private:
    struct Command {
        enum class Kind { kSolid, kTextured };
        Kind kind;
        Rect dst;
        Color color;                              // Both kinds: solid fill / texture tint.
        gfx::Texture* texture = nullptr;          // kTextured
        gfx::BlendMode blend = gfx::BlendMode::kAlpha;
        float u0 = 0, v0 = 0, u1 = 1, v1 = 1;     // kTextured
    };

    GlyphAtlas& atlas();

    gfx::Device& device_;
    std::unique_ptr<GlyphAtlas> atlas_;
    std::vector<std::unique_ptr<Image>> images_;
    std::vector<Command> commands_;
};

}  // namespace canvas
