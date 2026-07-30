#pragma once

#include <cstdint>
#include <span>

namespace gfx {

enum class TextureFormat { kRGBA8, kBGRA8 };

class Texture {
public:
    virtual ~Texture() = default;

    virtual int width() const = 0;
    virtual int height() const = 0;

    // Uploads pixels into the [x, x+w) x [y, y+h) sub-rectangle. `pixels` must be tightly packed
    // (no row padding) in `format`.
    virtual void update_region(int x,
                               int y,
                               int w,
                               int h,
                               TextureFormat format,
                               std::span<const uint8_t> pixels) = 0;
};

}  // namespace gfx
