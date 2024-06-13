#pragma once

#include "base/buffer.h"
#include "renderer/opengl_types.h"
#include "renderer/shader.h"
#include "renderer/text/glyph_cache.h"
#include "renderer/types.h"
#include <vector>

namespace renderer {

class TextRenderer {
public:
    TextRenderer(opengl::FunctionsGL* gl, GlyphCache& main_glyph_cache,
                 GlyphCache& ui_glyph_cache);
    ~TextRenderer();

    void setup();
    void renderText(const Size& size, const Point& scroll, const base::Buffer& buffer,
                    const Point& editor_offset, const CaretInfo& start_caret,
                    const CaretInfo& end_caret, int& longest_line_x, int line_number_offset,
                    int& end_caret_x);

private:
    static constexpr size_t kBatchMax = 0x10000;

    opengl::FunctionsGL* gl;
    GlyphCache& main_glyph_cache;
    GlyphCache& ui_glyph_cache;

    Shader shader_program;
    GLuint vao, vbo_instance, ebo;

    struct InstanceData {
        Vec2 coords;
        Vec4 glyph;
        Vec4 uv;
        Rgba color;
    };

    // TODO: Move batch code into a "Batch" class.
    std::vector<std::vector<InstanceData>> batch_instances;
};

}
