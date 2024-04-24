#include "base/rgb.h"
#include "font/rasterizer.h"
#include "renderer/opengl_error_util.h"
#include "renderer/opengl_types.h"
#include "selection_renderer.h"
#include <cstdint>

#include "build/buildflag.h"

namespace renderer {
// TODO: Rewrite this in a more idiomatic C++ way.
// Border flags.
#define LEFT 1
#define RIGHT 2
#define BOTTOM 4
#define TOP 8
#define BOTTOM_LEFT 16
#define BOTTOM_RIGHT 32
#define TOP_LEFT 64
#define TOP_RIGHT 128

namespace {
struct InstanceData {
    Vec2 coords;
    Vec2 bg_size;
    Rgba bg_color;
    Rgba bg_border_color;
};

// Reference Zed's implementation in //crates/gpui/src/text_system/line_layout.rs.
struct ShapedGlyph {
    uint32_t codepoint;
    size_t byte_offset;  // Byte offset within the line layout.
    Vec2 coords;
    Vec4 glyph;
    Vec4 uv;
    Vec2 bg_size;
    bool colored;
};
}

SelectionRenderer::SelectionRenderer() {}

SelectionRenderer::~SelectionRenderer() {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo_instance);
    glDeleteBuffers(1, &ebo);
}

void SelectionRenderer::setup(FontRasterizer& font_rasterizer) {
    std::string vert_source =
#include "shaders/selection_vert.glsl"
        ;
    std::string frag_source =
#include "shaders/selection_frag.glsl"
        ;

    shader_program.link(vert_source, frag_source);

    glUseProgram(shader_program.id);
    glUniform1f(glGetUniformLocation(shader_program.id, "line_height"),
                font_rasterizer.line_height);

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
    glBufferData(GL_ARRAY_BUFFER, sizeof(InstanceData) * BATCH_MAX, nullptr, GL_STATIC_DRAW);

    GLuint index = 0;

    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, 2, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
                          (void*)offsetof(InstanceData, coords));
    glVertexAttribDivisor(index++, 1);

    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, 2, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
                          (void*)offsetof(InstanceData, bg_size));
    glVertexAttribDivisor(index++, 1);

    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(InstanceData),
                          (void*)offsetof(InstanceData, bg_color));
    glVertexAttribDivisor(index++, 1);

    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(InstanceData),
                          (void*)offsetof(InstanceData, bg_border_color));
    glVertexAttribDivisor(index++, 1);

    // Unbind.
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void SelectionRenderer::render(Size& size, Point& scroll, Point& editor_offset,
                               FontRasterizer& font_rasterizer) {
    glUseProgram(shader_program.id);
    glUniform2f(glGetUniformLocation(shader_program.id, "resolution"), size.width, size.height);
    glUniform2f(glGetUniformLocation(shader_program.id, "scroll_offset"), scroll.x, scroll.y);
    glUniform2f(glGetUniformLocation(shader_program.id, "editor_offset"), editor_offset.x,
                editor_offset.y);

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vao);

    std::vector<InstanceData> instances;

    uint8_t border_flags = TOP | BOTTOM | LEFT | RIGHT;

    instances.push_back(InstanceData{
        .coords = Vec2{0, font_rasterizer.line_height * 4},
        .bg_size = Vec2{100, font_rasterizer.line_height},
        .bg_color = Rgba::fromRgb(colors::red, 255),
        // .bg_border_color = Rgba::fromRgb(colors::selection_border, border_flags),
        .bg_border_color = Rgba::fromRgb(colors::red, border_flags),
    });

    glBindBuffer(GL_ARRAY_BUFFER, vbo_instance);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(InstanceData) * instances.size(), &instances[0]);

    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, instances.size());

    // Unbind.
    glBindBuffer(GL_ARRAY_BUFFER, 0);  // Unbind.
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glCheckError();
}
}
