#pragma once

#include <span>

namespace gfx {

struct Color {
    float r, g, b, a;
};

struct Quad {
    float x, y, w, h;
    float r, g, b, a;
};

class Frame {
public:
    virtual ~Frame() = default;

    virtual void clear(const Color& c) = 0;
    virtual void set_viewport(int width, int height) = 0;
    virtual void draw_quads(std::span<const Quad> quads) = 0;
    virtual void present() = 0;
};

}  // namespace gfx
