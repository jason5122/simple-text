#include "gl/gl.h"
#include "gui/renderer/rect_renderer.h"
#include <cmath>

using namespace gl;

namespace {
const std::string kVertexShader =
#include "gui/renderer/shaders/rect_vert.glsl"
    ;
const std::string kFragmentShader =
#include "gui/renderer/shaders/rect_frag.glsl"
    ;
}  // namespace

namespace gui {

static_assert(!std::is_copy_constructible_v<RectRenderer>);
static_assert(!std::is_copy_assignable_v<RectRenderer>);
static_assert(std::is_move_constructible_v<RectRenderer>);
static_assert(std::is_move_assignable_v<RectRenderer>);

RectRenderer::RectRenderer() : shader_program{kVertexShader, kFragmentShader} {
    constexpr GLuint indices[] = {
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
    glVertexAttribPointer(index, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
                          (void*)offsetof(InstanceData, extra));
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
    : shader_program{std::move(other.shader_program)},
      vao{other.vao},
      vbo_instance{other.vbo_instance},
      ebo{other.ebo} {
    other.vao = 0;
    other.vbo_instance = 0;
    other.ebo = 0;
}

RectRenderer& RectRenderer::operator=(RectRenderer&& other) {
    if (&other != this) {
        shader_program = std::move(other.shader_program);
        vao = other.vao;
        vbo_instance = other.vbo_instance;
        ebo = other.ebo;
        other.vao = 0;
        other.vbo_instance = 0;
        other.ebo = 0;
    }
    return *this;
}

void RectRenderer::add_rect(const Point& coords,
                            const Size& size,
                            const Point& min_coords,
                            const Point& max_coords,
                            const Rgb& color,
                            Layer layer,
                            int corner_radius,
                            int tab_corner_radius,
                            int left_shadow,
                            int right_shadow) {
    int x = coords.x;
    int y = coords.y;
    int width = size.width;
    int height = size.height;

    int left_edge = x;
    int right_edge = left_edge + width;
    int top_edge = y;
    int bottom_edge = y + height;

    if (right_edge <= min_coords.x) return;
    if (left_edge > max_coords.x) return;
    if (bottom_edge <= min_coords.y) return;
    if (top_edge > max_coords.y) return;

    if (left_edge < min_coords.x) {
        int diff = min_coords.x - left_edge;
        width -= diff;
        x += diff;
    }
    if (right_edge > max_coords.x) {
        int diff = right_edge - max_coords.x;
        width -= diff;
    }
    if (top_edge < min_coords.y) {
        int diff = min_coords.y - top_edge;
        height -= diff;
        y += diff;
    }
    if (bottom_edge > max_coords.y) {
        int diff = bottom_edge - max_coords.y;
        height -= diff;
    }

    // TODO: Remove this.
    if (width <= 0 || height <= 0) {
        return;
    }

    auto& instances = layer == Layer::kBackground ? layer_one_instances : layer_two_instances;
    instances.emplace_back(InstanceData{
        .coords = {static_cast<float>(x), static_cast<float>(y)},
        .rect_size = {static_cast<float>(width), static_cast<float>(height)},
        .color = Rgba::fromRgb(color, 255),
        .extra =
            {
                .x = static_cast<float>(corner_radius),
                .y = static_cast<float>(tab_corner_radius),
                .z = static_cast<float>(left_shadow),
                .w = static_cast<float>(right_shadow),
            },
    });
}

void RectRenderer::flush(const Size& screen_size, Layer layer) {
    auto& instances = layer == Layer::kBackground ? layer_one_instances : layer_two_instances;
    if (instances.empty()) return;

    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE);

    GLuint shader_id = shader_program.id();
    glUseProgram(shader_id);
    glUniform2f(glGetUniformLocation(shader_id, "resolution"), screen_size.width,
                screen_size.height);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_instance);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(InstanceData) * instances.size(), instances.data());
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, instances.size());

    // Unbind.
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    instances.clear();
}

}  // namespace gui
