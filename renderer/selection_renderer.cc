#include "base/rgb.h"
#include "font/rasterizer.h"
#include "renderer/opengl_error_util.h"
#include "renderer/opengl_types.h"
#include "selection_renderer.h"
#include <cstdint>
#include <limits>

#include "build/buildflag.h"

namespace renderer {
// TODO: Rewrite this in a more idiomatic C++ way.
// Border flags.
#define LEFT 1
#define RIGHT 2
#define BOTTOM 4
#define TOP 8
#define BOTTOM_LEFT_INWARDS 16
#define BOTTOM_RIGHT_INWARDS 32
#define TOP_LEFT_INWARDS 64
#define TOP_RIGHT_INWARDS 128
#define BOTTOM_LEFT_OUTWARDS 256
#define BOTTOM_RIGHT_OUTWARDS 512
#define TOP_LEFT_OUTWARDS 1024
#define TOP_RIGHT_OUTWARDS 2048

namespace {
struct InstanceData {
    Vec2 coords;
    Vec2 bg_size;
    Rgba bg_color;
    Rgba bg_border_color;
    uint32_t border_flags;
    uint32_t bottom_border_offset;
    uint32_t top_border_offset;
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
    glUniform1i(glGetUniformLocation(shader_program.id, "corner_radius"), kCornerRadius);

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

    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, 4, GL_INT, GL_FALSE, sizeof(InstanceData),
                          (void*)offsetof(InstanceData, border_flags));
    glVertexAttribDivisor(index++, 1);

    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, 4, GL_INT, GL_FALSE, sizeof(InstanceData),
                          (void*)offsetof(InstanceData, bottom_border_offset));
    glVertexAttribDivisor(index++, 1);

    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, 4, GL_INT, GL_FALSE, sizeof(InstanceData),
                          (void*)offsetof(InstanceData, top_border_offset));
    glVertexAttribDivisor(index++, 1);

    // Unbind.
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void SelectionRenderer::render(Size& size, Point& scroll, Point& editor_offset,
                               FontRasterizer& font_rasterizer,
                               std::vector<Selection>& selections) {
    glUseProgram(shader_program.id);
    glUniform2f(glGetUniformLocation(shader_program.id, "resolution"), size.width, size.height);
    glUniform2f(glGetUniformLocation(shader_program.id, "scroll_offset"), scroll.x, scroll.y);
    glUniform2f(glGetUniformLocation(shader_program.id, "editor_offset"), editor_offset.x,
                editor_offset.y);

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vao);

    std::vector<InstanceData> instances;

    auto create = [&instances, &font_rasterizer](
                      int start, int end, int line,
                      uint32_t border_flags = LEFT | RIGHT | TOP | BOTTOM,
                      uint32_t bottom_border_offset = 0, uint32_t top_border_offset = 0,
                      Rgb bg_color = colors::selection_unfocused) {
        constexpr int border_thickness = 2;
        instances.emplace_back(InstanceData{
            .coords =
                {
                    .x = static_cast<float>(start),
                    .y = font_rasterizer.line_height * line,
                },
            .bg_size =
                {
                    .x = static_cast<float>(end - start),
                    .y = font_rasterizer.line_height + border_thickness,
                    // .y = font_rasterizer.line_height,
                },
            .bg_color = Rgba::fromRgb(bg_color, 255),
            .bg_border_color = Rgba::fromRgb(colors::red, 0),
            .border_flags = border_flags,
            .bottom_border_offset = bottom_border_offset,
            .top_border_offset = top_border_offset,
        });
    };

    size_t selections_size = selections.size();
    for (size_t i = 0; i < selections_size; i++) {
        uint32_t flags = LEFT | RIGHT;
        uint32_t bottom_border_offset = 0;
        uint32_t top_border_offset = 0;

        if (i == 0) {
            flags |= TOP | TOP_LEFT_INWARDS | TOP_RIGHT_INWARDS;
        }
        if (i == selections_size - 1) {
            flags |= BOTTOM | BOTTOM_LEFT_INWARDS | BOTTOM_RIGHT_INWARDS;
        }

        if (i > 0) {
            if (selections[i].end > selections[i - 1].end) {
                flags |= TOP_RIGHT_INWARDS;

                flags |= TOP;
                top_border_offset = selections[i - 1].end;
            } else if (selections[i].end < selections[i - 1].end) {
                flags |= TOP_RIGHT_OUTWARDS;
            }
        }
        if (i + 1 < selections_size) {
            if (selections[i].start > selections[i + 1].start) {
                flags |= BOTTOM_LEFT_OUTWARDS;
            }

            if (selections[i].end > selections[i + 1].end) {
                flags |= BOTTOM_RIGHT_INWARDS;

                flags |= BOTTOM;
                bottom_border_offset = selections[i + 1].end;
            } else if (selections[i].end < selections[i + 1].end) {
                flags |= BOTTOM_RIGHT_OUTWARDS;
            }
        }

        create(selections[i].start, selections[i].end, selections[i].line, flags,
               bottom_border_offset, top_border_offset);
    }

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
