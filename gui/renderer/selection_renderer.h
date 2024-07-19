#pragma once

#include "gui/renderer/shader.h"
#include "gui/renderer/text/glyph_cache.h"
#include "gui/renderer/text/line_layout.h"
#include "gui/renderer/types.h"
#include "util/non_copyable.h"

namespace gui {

class SelectionRenderer : util::NonCopyable {
public:
    SelectionRenderer(GlyphCache& main_glyph_cache);
    ~SelectionRenderer();
    SelectionRenderer(SelectionRenderer&& other);
    SelectionRenderer& operator=(SelectionRenderer&& other);

    void renderSelections(const Point& offset,
                          const LineLayout& line_layout,
                          std::vector<LineLayout::Token>::const_iterator start_caret,
                          std::vector<LineLayout::Token>::const_iterator end_caret);
    void render(const Size& screen_size, int rendering_pass);
    void destroyInstances();

private:
    static constexpr size_t kBatchMax = 0x10000;
    static constexpr int kCornerRadius = 6;
    static constexpr int kBorderThickness = 2;

    GlyphCache& main_glyph_cache;

    struct Selection {
        int line;
        int start;
        int end;
    };

    std::vector<Selection> getSelections(
        const LineLayout& line_layout,
        std::vector<LineLayout::Token>::const_iterator start_caret,
        std::vector<LineLayout::Token>::const_iterator end_caret);

    Shader shader_program;
    GLuint vao = 0;
    GLuint vbo_instance = 0;
    GLuint ebo = 0;

    struct InstanceData {
        Vec2 coords;
        Vec2 size;
        Rgba color;
        Rgba border_color;
        // <border_flags, bottom_border_offset, top_border_offset, hide_background>
        IVec4 border_info;
    };
    std::vector<InstanceData> instances;

    enum BorderFlags {
        kLeft = 1,
        kRight = 1 << 1,
        kBottom = 1 << 2,
        kTop = 1 << 3,
        kBottomLeftInwards = 1 << 4,
        kBottomRightInwards = 1 << 5,
        kTopLeftInwards = 1 << 6,
        kTopRightInwards = 1 << 7,
        kBottomLeftOutwards = 1 << 8,
        kBottomRightOutwards = 1 << 9,
        kTopLeftOutwards = 1 << 10,
        kTopRightOutwards = 1 << 11,
    };
};

}