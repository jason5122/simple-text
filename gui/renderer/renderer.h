#pragma once

#include "gui/renderer/image_renderer.h"
#include "gui/renderer/rect_renderer.h"
#include "gui/renderer/selection_renderer.h"
#include "gui/renderer/text_renderer.h"

namespace gui {

class Renderer {
public:
    static Renderer& instance();

    TextRenderer& getTextRenderer();
    RectRenderer& getRectRenderer();
    SelectionRenderer& getSelectionRenderer();
    ImageRenderer& getImageRenderer();

    void flush(const app::Size& size);

private:
    Renderer();

    TextRenderer text_renderer;
    RectRenderer rect_renderer;
    SelectionRenderer selection_renderer;
    ImageRenderer image_renderer;
};

}  // namespace gui
