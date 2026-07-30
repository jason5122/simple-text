#pragma once

#include "gfx/surface.h"
#include "gfx/texture.h"
#include <cstdint>
#include <memory>
#include <span>

namespace gfx {

enum class Backend { kOpenGL, kMetal };

class Device {
public:
    virtual ~Device() = default;

    virtual std::unique_ptr<Surface> create_surface(int width, int height) = 0;

    // Creates a texture of the given size and format. `pixels`, if non-empty, initializes the full
    // image (tightly packed); if empty, the texture is allocated with undefined contents (an empty
    // atlas page to be filled via Texture::update_region).
    virtual std::unique_ptr<Texture> create_texture(int width,
                                                    int height,
                                                    TextureFormat format,
                                                    std::span<const uint8_t> pixels = {}) = 0;
};

std::unique_ptr<Device> create_device(Backend backend);

}  // namespace gfx
