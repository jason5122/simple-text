#pragma once

#include "gfx/frame.h"
#include <memory>

namespace gfx {

class Surface {
public:
    virtual ~Surface() = default;

    virtual std::unique_ptr<Frame> begin_frame() = 0;
    virtual void resize(int width, int height) = 0;
    virtual int width() const = 0;
    virtual int height() const = 0;
};

}  // namespace gfx
