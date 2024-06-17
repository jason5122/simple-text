#include "opengl/functions_gl_enums.h"
#include "rect_renderer.h"
#include <vector>

namespace {
const std::string kVertexShaderSource =
#include "renderer/shaders/rect_vert.glsl"
    ;
const std::string kFragmentShaderSource =
#include "renderer/shaders/rect_frag.glsl"
    ;
}

namespace renderer {

RectRenderer::RectRenderer(std::shared_ptr<opengl::FunctionsGL> shared_gl)
    : gl{std::move(shared_gl)}, shader_program{gl, kVertexShaderSource, kFragmentShaderSource} {
    gl->genVertexArrays(1, &vao);
    gl->genBuffers(1, &vbo_instance);
    gl->genBuffers(1, &ebo);

    GLuint indices[] = {
        0, 1, 3,  // First triangle.
        1, 2, 3,  // Second triangle.
    };

    gl->bindVertexArray(vao);

    gl->bindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    gl->bufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    gl->bindBuffer(GL_ARRAY_BUFFER, vbo_instance);
    gl->bufferData(GL_ARRAY_BUFFER, sizeof(InstanceData) * kBatchMax, nullptr, GL_STATIC_DRAW);

    GLuint index = 0;

    gl->enableVertexAttribArray(index);
    gl->vertexAttribPointer(index, 2, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
                            (void*)offsetof(InstanceData, coords));
    gl->vertexAttribDivisor(index++, 1);

    gl->enableVertexAttribArray(index);
    gl->vertexAttribPointer(index, 2, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
                            (void*)offsetof(InstanceData, rect_size));
    gl->vertexAttribDivisor(index++, 1);

    gl->enableVertexAttribArray(index);
    gl->vertexAttribPointer(index, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(InstanceData),
                            (void*)offsetof(InstanceData, color));
    gl->vertexAttribDivisor(index++, 1);

    gl->enableVertexAttribArray(index);
    gl->vertexAttribPointer(index, 1, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
                            (void*)offsetof(InstanceData, corner_radius));
    gl->vertexAttribDivisor(index++, 1);

    gl->enableVertexAttribArray(index);
    gl->vertexAttribPointer(index, 1, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
                            (void*)offsetof(InstanceData, tab_corner_radius));
    gl->vertexAttribDivisor(index++, 1);

    // Unbind.
    gl->bindVertexArray(0);
    gl->bindBuffer(GL_ARRAY_BUFFER, 0);
    gl->bindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

RectRenderer::~RectRenderer() {
    gl->deleteVertexArrays(1, &vao);
    gl->deleteBuffers(1, &vbo_instance);
    gl->deleteBuffers(1, &ebo);
}

RectRenderer::RectRenderer(RectRenderer&& other)
    : vao{other.vao}, vbo_instance{other.vbo_instance}, ebo{other.ebo}, gl{other.gl},
      shader_program{std::move(other.shader_program)} {
    other.vao = 0;
    other.vbo_instance = 0;
    other.ebo = 0;
}

RectRenderer& RectRenderer::operator=(RectRenderer&& other) {
    if (&other != this) {
        vao = other.vao;
        vbo_instance = other.vbo_instance;
        ebo = other.ebo;
        shader_program = std::move(other.shader_program);
        other.vao = 0;
        other.vbo_instance = 0;
        other.ebo = 0;
    }
    return *this;
}

void RectRenderer::draw(const Size& size, const Point& scroll, const Point& end_caret_pos,
                        float line_height, size_t line_count, float longest_x,
                        const Point& editor_offset, float status_bar_height) {
    GLuint shader_id = shader_program.id();
    gl->useProgram(shader_id);
    gl->uniform2f(gl->getUniformLocation(shader_id, "resolution"), size.width, size.height);
    gl->uniform2f(gl->getUniformLocation(shader_id, "scroll_offset"), scroll.x, scroll.y);
    gl->uniform2f(gl->getUniformLocation(shader_id, "editor_offset"), editor_offset.x,
                  editor_offset.y);
    gl->bindVertexArray(vao);

    int caret_width = 4;
    int caret_height = line_height;

    int extra_padding = 8;
    caret_height += extra_padding * 2;

    Rgba editor_bg_color{253, 253, 253, 255};

    std::vector<InstanceData> instances;

    // line_count -= 1;  // TODO: Merge this with EditorView.

    float editor_width = size.width - editor_offset.x;
    float editor_height = size.height - editor_offset.y - status_bar_height;

    // TODO: Add this to parameters.
    int line_number_offset = 100;

    // Add caret.
    instances.emplace_back(InstanceData{
        .coords =
            Vec2{
                .x = static_cast<float>(end_caret_pos.x - caret_width / 2 - scroll.x +
                                        line_number_offset),
                .y = static_cast<float>(end_caret_pos.y - extra_padding - scroll.y),
            },
        .rect_size =
            Vec2{
                .x = static_cast<float>(caret_width),
                .y = static_cast<float>(caret_height),
            },
        .color = Rgba{95, 180, 180, 255},
    });

    // // Add vertical scroll bar.
    // if (line_count > 0) {
    //     float vertical_scroll_bar_width = 15;
    //     float total_y = (line_count + (editor_height / line_height)) * line_height;
    //     float vertical_scroll_bar_height = editor_height * (editor_height / total_y);
    //     float vertical_scroll_bar_position_percentage = scroll.y / (line_count * line_height);
    //     instances.emplace_back(InstanceData{
    //         .coords =
    //             Vec2{
    //                 editor_width - vertical_scroll_bar_width,
    //                 (editor_height - vertical_scroll_bar_height) *
    //                     vertical_scroll_bar_position_percentage,
    //             },
    //         .rect_size = Vec2{vertical_scroll_bar_width, vertical_scroll_bar_height},
    //         .color = Rgba::fromRgb(color_scheme.scroll_bar, 255),
    //         .corner_radius = 5,
    //     });
    // }

    // // Add horizontal scroll bar.
    // float horizontal_scroll_bar_width = editor_width * (editor_width / longest_x);
    // float horizontal_scroll_bar_height = 15;
    // float horizontal_scroll_bar_position_percentage = scroll.x / (longest_x - editor_width);
    // if (horizontal_scroll_bar_width < editor_width) {
    //     instances.emplace_back(InstanceData{
    //         .coords = Vec2{(editor_width - horizontal_scroll_bar_width) *
    //                            horizontal_scroll_bar_position_percentage,
    //                        editor_height - horizontal_scroll_bar_height},
    //         .rect_size = Vec2{horizontal_scroll_bar_width, horizontal_scroll_bar_height},
    //         .color = Rgba::fromRgb(color_scheme.scroll_bar, 255),
    //         .corner_radius = 5,
    //     });
    // }

    // // Add tab bar.
    instances.emplace_back(InstanceData{
        .coords = Vec2{0, static_cast<float>(-editor_offset.y)},
        .rect_size = Vec2{static_cast<float>(size.width), static_cast<float>(editor_offset.y)},
        // .color = Rgba::fromRgb(color_scheme.tab_bar, 255),
        .color = Rgba{190, 190, 190, 255},
    });

    // float tab_height = editor_offset.y - 5;  // Leave padding between window title bar and tab.
    // float tab_corner_radius = 10;

    // int total_x = 0;

    // for (size_t i = 0; i < tab_title_widths.size(); i++) {
    //     Rgba color = i == tab_index ? editor_bg_color : Rgba{100, 100, 100, 255};
    //     tab_title_x_coords.emplace_back(total_x + tab_corner_radius);

    //     int tab_width = tab_title_widths[i] + tab_corner_radius * 2;
    //     tab_width = std::max(kMinTabWidth, tab_width);

    //     actual_tab_title_widths.emplace_back(tab_width - tab_corner_radius * 2);

    //     instances.emplace_back(InstanceData{
    //         .coords = Vec2{static_cast<float>(total_x), 0 - tab_height},
    //         .rect_size = Vec2{static_cast<float>(tab_width), tab_height},
    //         .color = color,
    //         .tab_corner_radius = tab_corner_radius,
    //     });

    //     total_x += tab_width;
    // }

    // Add side bar.
    instances.emplace_back(InstanceData{
        .coords = {static_cast<float>(-editor_offset.x), static_cast<float>(-editor_offset.y)},
        .rect_size = {static_cast<float>(editor_offset.x), static_cast<float>(size.height)},
        // .color = Rgba::fromRgb(color_scheme.side_bar, 255),
        .color = Rgba{235, 237, 239, 255},
    });

    // Add status bar.
    instances.emplace_back(InstanceData{
        .coords = Vec2{static_cast<float>(-editor_offset.x), editor_height},
        .rect_size = Vec2{static_cast<float>(size.width), status_bar_height},
        // .color = Rgba::fromRgb(color_scheme.status_bar, 255),
        .color = Rgba{199, 203, 209, 255},
    });

    gl->bindBuffer(GL_ARRAY_BUFFER, vbo_instance);
    gl->bufferSubData(GL_ARRAY_BUFFER, 0, sizeof(InstanceData) * instances.size(), &instances[0]);

    gl->drawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, instances.size());

    // Unbind.
    gl->bindBuffer(GL_ARRAY_BUFFER, 0);
    gl->bindVertexArray(0);
}

}
