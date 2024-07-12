#pragma once

#include "base/buffer.h"
#include "base/utf8_string.h"
#include "gui/renderer/opengl_types.h"
#include "gui/renderer/shader.h"
#include "gui/renderer/text/glyph_cache.h"
#include "gui/renderer/text/line_layout.h"
#include "gui/renderer/types.h"
#include <vector>

namespace gui {

class TextRenderer {
public:
    TextRenderer(GlyphCache& main_glyph_cache, GlyphCache& ui_glyph_cache);
    ~TextRenderer();
    TextRenderer(TextRenderer&& other);
    TextRenderer& operator=(TextRenderer&& other);

    void layout(const base::Buffer& buffer);
    void renderText(size_t start_line,
                    size_t end_line,
                    const Point& offset,
                    const base::Buffer& buffer);
    void addUiText(const Point& coords, const Rgb& color, const base::Utf8String& str8);
    void flush(const Size& screen_size, bool use_main_glyph_cache);
    int lineHeight();
    int uiLineHeight();
    LineLayout& getLineLayout();

private:
    static constexpr size_t kBatchMax = 0x10000;

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

    LineLayout line_layout;

    // TODO: Move batch code into a "Batch" class.
    std::vector<std::vector<InstanceData>> main_batch_instances;
    std::vector<std::vector<InstanceData>> ui_batch_instances;

    void insertIntoBatch(size_t page, const InstanceData& instance, bool use_main_glyph_cache);
};

}
