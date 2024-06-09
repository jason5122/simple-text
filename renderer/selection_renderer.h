#pragma once

#include "renderer/opengl_functions.h"
#include "renderer/opengl_types.h"
#include "renderer/shader.h"
#include "renderer/text/glyph_cache.h"
#include "renderer/types.h"
#include "util/non_copyable.h"

namespace renderer {

class SelectionRenderer : util::NonMovable {
public:
    struct Selection {
        int line;
        int start;
        int end;
    };

    SelectionRenderer();
    ~SelectionRenderer();
    void setup();
    void createInstances(Size& size, Point& scroll, Point& editor_offset,
                         renderer::GlyphCache& main_glyph_cache,
                         std::vector<Selection>& selections, int line_number_offset);
    void render(int rendering_pass);
    void destroyInstances();

private:
    static constexpr int kBatchMax = 65536;
    static constexpr int kCornerRadius = 6;
    static constexpr int kBorderThickness = 2;

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

    Shader shader_program;
    GLuint vao, vbo_instance, ebo;

    struct InstanceData {
        Vec2 coords;
        Vec2 size;
        Rgba color;
        Rgba border_color;
        // <border_flags, bottom_border_offset, top_border_offset, hide_background>
        IVec4 border_info;
    };
    std::vector<InstanceData> instances;
};

}
