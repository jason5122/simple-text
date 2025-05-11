#pragma once

#include "gui/renderer/types.h"
#include "gui/types.h"

namespace gui {

class RendererLite {
public:
    inline static RendererLite& instance() {
        static RendererLite renderer;
        return renderer;
    }

    void flush(const Size& size, const Rgb& color);

private:
    RendererLite();
};

}  // namespace gui
