#pragma once

#include "gui/app/types.h"
#include "gui/renderer/shader.h"
#include "gui/renderer/texture_cache.h"
#include "gui/renderer/types.h"

#include <functional>
#include <vector>

namespace gui {

class TextureRenderer : util::NonCopyable {
public:
    TextureRenderer();
    ~TextureRenderer();
    TextureRenderer(TextureRenderer&& other);
    TextureRenderer& operator=(TextureRenderer&& other);

    void addLineLayout(const font::LineLayout& line_layout,
                       const Point& coords,
                       const Point& min_coords,
                       const Point& max_coords,
                       const std::function<Rgb(size_t)>& highlight_callback);
    void addImage(size_t image_index, const Point& coords, const Rgb& color);
    void addColorImage(size_t image_index, const Point& coords);
    void flush(const Size& screen_size);

private:
    static constexpr size_t kBatchMax = 0x10000;

    Shader shader_program;
    GLuint vao = 0;
    GLuint vbo_instance = 0;
    GLuint ebo = 0;

    struct InstanceData {
        Vec2 coords;
        Vec4 glyph;
        Vec4 uv;
        Rgba color;
    };

    std::vector<std::vector<InstanceData>> batches;

    void insertIntoBatch(size_t page, const InstanceData& instance);

    enum InstanceKind {
        kPlainTexture = 0,
        kColoredText = 1,
        kColoredImage = 2,
    };

    // DEBUG: Draws texture atlases.
    friend class AtlasWidget;
    void renderAtlasPage(size_t page,
                         const Point& coords,
                         const Point& min_coords,
                         const Point& max_coords);
};

}  // namespace gui
