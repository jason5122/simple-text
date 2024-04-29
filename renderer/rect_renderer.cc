#include "base/rgb.h"
#include "rect_renderer.h"
#include "renderer/opengl_error_util.h"
#include "renderer/opengl_types.h"
#include <vector>

namespace renderer {
namespace {
struct InstanceData {
    Vec2 coords;
    Vec2 rect_size;
    Rgba color;
    float corner_radius = 0;
    float tab_corner_radius = 0;
};
}

RectRenderer::RectRenderer() {}

RectRenderer::~RectRenderer() {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo_instance);
    glDeleteBuffers(1, &ebo);
}

void RectRenderer::setup() {
    std::string vert_source =
#include "shaders/rect_vert.glsl"
        ;
    std::string frag_source =
#include "shaders/rect_frag.glsl"
        ;

    shader_program.link(vert_source, frag_source);

    GLuint indices[] = {
        0, 1, 3,  // First triangle.
        1, 2, 3,  // Second triangle.
    };

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo_instance);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_instance);
    glBufferData(GL_ARRAY_BUFFER, sizeof(InstanceData) * kBatchMax, nullptr, GL_STATIC_DRAW);

    GLuint index = 0;

    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, 2, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
                          (void*)offsetof(InstanceData, coords));
    glVertexAttribDivisor(index++, 1);

    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, 2, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
                          (void*)offsetof(InstanceData, rect_size));
    glVertexAttribDivisor(index++, 1);

    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(InstanceData),
                          (void*)offsetof(InstanceData, color));
    glVertexAttribDivisor(index++, 1);

    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, 1, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
                          (void*)offsetof(InstanceData, corner_radius));
    glVertexAttribDivisor(index++, 1);

    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, 1, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
                          (void*)offsetof(InstanceData, tab_corner_radius));
    glVertexAttribDivisor(index++, 1);

    // Unbind.
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void RectRenderer::draw(Size& size, Point& scroll, CaretInfo& end_caret, float line_height,
                        size_t line_count, float longest_x, Point& editor_offset,
                        float status_bar_height, config::ColorScheme& color_scheme) {
    glUseProgram(shader_program.id);
    glUniform2f(glGetUniformLocation(shader_program.id, "resolution"), size.width, size.height);
    glUniform2f(glGetUniformLocation(shader_program.id, "scroll_offset"), scroll.x, scroll.y);
    glUniform2f(glGetUniformLocation(shader_program.id, "editor_offset"), editor_offset.x,
                editor_offset.y);
    glBindVertexArray(vao);

    float caret_width = 4;
    float caret_height = line_height;

    float caret_x = end_caret.x - caret_width / 2;

    int extra_padding = 8;
    float caret_y = end_caret.line * line_height;
    caret_y -= extra_padding;
    caret_height += extra_padding * 2;

    Rgba editor_bg_color = Rgba::fromRgb(color_scheme.background, 255);

    std::vector<InstanceData> instances;

    // line_count -= 1;  // TODO: Merge this with EditorView.

    float editor_width = size.width - editor_offset.x;
    float editor_height = size.height - editor_offset.y - status_bar_height;

    // Add caret.
    if ((scroll.x < caret_x + caret_width && caret_x < scroll.x + editor_width) &&
        (scroll.y < caret_y + caret_height && caret_y < scroll.y + editor_height)) {
        instances.push_back(InstanceData{
            .coords = Vec2{caret_x - scroll.x, caret_y - scroll.y},
            .rect_size = Vec2{caret_width, caret_height},
            .color = Rgba::fromRgb(color_scheme.caret, 255),
        });
    }

    // Add vertical scroll bar.
    if (line_count > 0) {
        float vertical_scroll_bar_width = 15;
        float total_y = (line_count + (editor_height / line_height)) * line_height;
        float vertical_scroll_bar_height = editor_height * (editor_height / total_y);
        float vertical_scroll_bar_position_percentage = scroll.y / (line_count * line_height);
        instances.push_back(InstanceData{
            .coords =
                Vec2{
                    editor_width - vertical_scroll_bar_width,
                    (editor_height - vertical_scroll_bar_height) *
                        vertical_scroll_bar_position_percentage,
                },
            .rect_size = Vec2{vertical_scroll_bar_width, vertical_scroll_bar_height},
            .color = Rgba::fromRgb(color_scheme.scroll_bar, 255),
            .corner_radius = 5,
        });
    }

    // Add horizontal scroll bar.
    float horizontal_scroll_bar_width = editor_width * (editor_width / longest_x);
    float horizontal_scroll_bar_height = 15;
    float horizontal_scroll_bar_position_percentage = scroll.x / (longest_x - editor_width);
    if (horizontal_scroll_bar_width < editor_width) {
        instances.push_back(InstanceData{
            .coords = Vec2{(editor_width - horizontal_scroll_bar_width) *
                               horizontal_scroll_bar_position_percentage,
                           editor_height - horizontal_scroll_bar_height},
            .rect_size = Vec2{horizontal_scroll_bar_width, horizontal_scroll_bar_height},
            .color = Rgba::fromRgb(color_scheme.scroll_bar, 255),
            .corner_radius = 5,
        });
    }

    // Add tab bar.
    instances.push_back(InstanceData{
        .coords = Vec2{0, 0 - editor_offset.y},
        .rect_size = Vec2{static_cast<float>(size.width), editor_offset.y},
        .color = Rgba::fromRgb(color_scheme.tab_bar, 255),
    });

    float tab_width = 350;
    float tab_height = editor_offset.y - 5;  // Leave padding between window title bar and tab.
    float tab_corner_radius = 10;

    // Add tab 1.
    instances.push_back(InstanceData{
        .coords = Vec2{0, 0 - tab_height},
        .rect_size = Vec2{tab_width, tab_height},
        .color = editor_bg_color,
        .tab_corner_radius = tab_corner_radius,
    });

    // // Add tab 2.
    // instances.push_back(InstanceData{
    //     .coords = Vec2{tab_width * 1, 0 - tab_height},
    //     .rect_size = Vec2{tab_width, tab_height},
    //     .color = editor_bg_color,
    //     .tab_corner_radius = tab_corner_radius,
    // });

    // // Add tab 3.
    // instances.push_back(InstanceData{
    //     .coords = Vec2{tab_width * 2, 0 - tab_height},
    //     .rect_size = Vec2{tab_width, tab_height},
    //     .color = editor_bg_color,
    //     .tab_corner_radius = tab_corner_radius,
    // });

    // Add side bar.
    instances.push_back(InstanceData{
        .coords = {0 - editor_offset.x, 0 - editor_offset.y},
        .rect_size = {editor_offset.x, static_cast<float>(size.height)},
        .color = Rgba::fromRgb(color_scheme.side_bar, 255),
    });

    // Add status bar.
    instances.push_back(InstanceData{
        .coords = Vec2{0 - editor_offset.x, editor_height},
        .rect_size = Vec2{static_cast<float>(size.width), status_bar_height},
        .color = Rgba::fromRgb(color_scheme.status_bar, 255),
    });

    glBindBuffer(GL_ARRAY_BUFFER, vbo_instance);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(InstanceData) * instances.size(), &instances[0]);

    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, instances.size());

    // Unbind.
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glCheckError();
}
}
