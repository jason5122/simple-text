#pragma once

#include "gui/renderer/shader.h"
#include "gui/renderer/types.h"
#include "gui/types.h"
#include "util/non_copyable.h"

#include <vector>

namespace gui {

class SelectionRenderer : util::NonCopyable {
public:
    SelectionRenderer();
    ~SelectionRenderer();
    SelectionRenderer(SelectionRenderer&& other);
    SelectionRenderer& operator=(SelectionRenderer&& other);

    struct Selection {
        int line;
        int start;
        int end;
    };
    void add_selections(const std::vector<Selection>& sels,
                       const Point& offset,
                       int line_height,
                       const Point& min_coords,
                       const Point& max_coords);
    void flush(const Size& screen_size);

private:
    static constexpr size_t kBatchMax = 0x10000;
    static constexpr int kCornerRadius = 6;
    static constexpr int kBorderThickness = 2;

    // static constexpr Rgba kSelectionColor = {227, 230, 232, 0};        // Light.
    static constexpr Rgba kSelectionColor = {77, 88, 100, 0};  // Dark.
    // static constexpr Rgba kSelectionBorderColor = {212, 217, 221, 0};  // Light.
    static constexpr Rgba kSelectionBorderColor = {100, 115, 130, 0};  // Dark.

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
        // <min_x, min_y, max_x, max_y>
        Vec4 clip_rect;
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

}  // namespace gui
