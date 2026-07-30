#pragma once

#include "gfx/texture.h"
#include <span>

namespace gfx {

class Device;

struct Color {
    float r, g, b, a;
};

struct Quad {
    float x, y, w, h;
    float r, g, b, a;
};

enum class BlendMode {
    // Straight (non-premultiplied) alpha: src*a + dst*(1-a). For solids and straight-alpha images.
    kAlpha,
    // Premultiplied alpha: src + dst*(1-a). For glyph coverage and premultiplied images.
    kPremultiplied,
};

struct TexturedQuad {
    float x, y, w, h;        // Destination rect in pixels.
    float u0, v0, u1, v1;    // Source texture coords in [0, 1].
    float r, g, b, a;        // Tint, multiplied with the sampled texel.
};

class Frame {
public:
    virtual ~Frame() = default;

    // The device that produced this frame, for allocating GPU resources during a draw.
    virtual Device& device() = 0;

    virtual void clear(const Color& c) = 0;
    virtual void draw_quads(std::span<const Quad> quads, float transform_x, float transform_y) = 0;
    virtual void draw_textured_quads(std::span<const TexturedQuad> quads,
                                     Texture& texture,
                                     BlendMode blend,
                                     float transform_x,
                                     float transform_y) = 0;
    virtual void finish() = 0;
};

}  // namespace gfx
