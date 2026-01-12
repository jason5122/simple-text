#pragma once

#include "gfx/surface.h"
#include <memory>

namespace gfx {

enum class Backend {
    kOpenGL,
};

class Device {
public:
    virtual ~Device() = default;

    virtual std::unique_ptr<Surface> create_surface(int width, int height) = 0;
};

std::unique_ptr<Device> create_device(Backend backend);

}  // namespace gfx
