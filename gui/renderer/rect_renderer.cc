#include "rect_renderer.h"
#include <cmath>

#include "opengl/gl.h"
using namespace opengl;

namespace {
const std::string kVertexShaderSource =
#include "gui/renderer/shaders/rect_vert.glsl"
    ;
const std::string kFragmentShaderSource =
#include "gui/renderer/shaders/rect_frag.glsl"
    ;
}

namespace renderer {

RectRenderer::RectRenderer() : shader_program{kVertexShaderSource, kFragmentShaderSource} {
    instances.reserve(kBatchMax);

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo_instance);
    glGenBuffers(1, &ebo);

    GLuint indices[] = {
        0, 1, 3,  // First triangle.
        1, 2, 3,  // Second triangle.
    };

    glBindVertexArray(vao);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_instance);
    glBufferData(GL_ARRAY_BUFFER, sizeof(InstanceData) * kBatchMax, nullptr, GL_STREAM_DRAW);

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

RectRenderer::~RectRenderer() {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo_instance);
    glDeleteBuffers(1, &ebo);
}

RectRenderer::RectRenderer(RectRenderer&& other)
    : vao{other.vao},
      vbo_instance{other.vbo_instance},
      ebo{other.ebo},
      shader_program{std::move(other.shader_program)} {
    instances.reserve(kBatchMax);
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

void RectRenderer::draw(const Size& size,
                        const Point& scroll,
                        const Point& end_caret_pos,
                        float line_height,
                        size_t line_count,
                        float longest_x,
                        const Point& editor_offset,
                        int status_bar_height) {
    int caret_width = 4;
    int caret_height = line_height;

    int extra_padding = 8;
    caret_height += extra_padding * 2;

    int editor_width = size.width;
    int editor_height = size.height - status_bar_height;

    // TODO: Add this to parameters.
    int line_number_offset = 100;

    // Add caret.
    const Point caret_pos{
        .x = end_caret_pos.x - caret_width / 2 - scroll.x + editor_offset.x + line_number_offset,
        .y = end_caret_pos.y - extra_padding - scroll.y + editor_offset.y,
    };
    addRect(caret_pos, {caret_width, caret_height}, Rgba{95, 180, 180, 255});

    // Add vertical scroll bar.
    if (line_count > 0) {
        float vertical_scroll_bar_width = 15;
        float total_y = (line_count + (editor_height / line_height)) * line_height;
        float vertical_scroll_bar_height = editor_height * (editor_height / total_y);
        float vertical_scroll_bar_position_percentage = scroll.y / (line_count * line_height);
        instances.emplace_back(InstanceData{
            // TODO: Round floats.
            .coords =
                Vec2{
                    .x = editor_width - vertical_scroll_bar_width,
                    .y = std::round((editor_height - vertical_scroll_bar_height) *
                                    vertical_scroll_bar_position_percentage),
                },
            .rect_size = Vec2{vertical_scroll_bar_width, vertical_scroll_bar_height},
            .color = {190, 190, 190, 255},
            .corner_radius = 5,
        });
    }

    // Add horizontal scroll bar.
    float horizontal_scroll_bar_width = editor_width * (editor_width / longest_x);
    float horizontal_scroll_bar_height = 15;
    float horizontal_scroll_bar_position_percentage = scroll.x / (longest_x - editor_width);
    if (horizontal_scroll_bar_width < editor_width) {
        instances.emplace_back(InstanceData{
            // TODO: Round floats.
            .coords =
                Vec2{
                    .x = std::round((editor_width - horizontal_scroll_bar_width) *
                                    horizontal_scroll_bar_position_percentage),
                    .y = editor_height - horizontal_scroll_bar_height,
                },
            .rect_size = Vec2{horizontal_scroll_bar_width, horizontal_scroll_bar_height},
            .color = {190, 190, 190, 255},
            .corner_radius = 5,
        });
    }

    // Add tab bar.
    addRect({0, 0}, {size.width, editor_offset.y}, Rgba{190, 190, 190, 255});

    float tab_height = editor_offset.y - 5;  // Leave padding between window title bar and tab.
    float tab_corner_radius = 10;

    instances.emplace_back(InstanceData{
        .coords = Vec2{0, -tab_height},
        .rect_size = Vec2{kMinTabWidth, tab_height},
        .color = Rgba{100, 100, 100, 255},
        .tab_corner_radius = tab_corner_radius,
    });

    // Add side bar.
    addRect({0, 0}, {editor_offset.x, size.height}, Rgba{235, 237, 239, 255});

    // Add status bar.
    addRect({0, editor_height}, size, Rgba{199, 203, 209, 255});
}

void RectRenderer::addRect(const Point& coords,
                           const Size& size,
                           const Rgba& color,
                           int corner_radius,
                           int tab_corner_radius) {
    instances.emplace_back(InstanceData{
        .coords = Vec2{static_cast<float>(coords.x), static_cast<float>(coords.y)},
        .rect_size = Vec2{static_cast<float>(size.width), static_cast<float>(size.height)},
        .color = color,
        .corner_radius = static_cast<float>(corner_radius),
        .tab_corner_radius = static_cast<float>(tab_corner_radius),
    });
}

void RectRenderer::flush(const Size& screen_size) {
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE);

    GLuint shader_id = shader_program.id();
    glUseProgram(shader_id);
    glUniform2f(glGetUniformLocation(shader_id, "resolution"), screen_size.width,
                screen_size.height);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_instance);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(InstanceData) * instances.size(), &instances[0]);
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, instances.size());

    // Unbind.
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    instances.clear();
}

}
