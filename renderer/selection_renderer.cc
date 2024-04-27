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

    auto create = [&instances,
                   &font_rasterizer](int start, int end, int line,
                                     uint32_t border_flags = LEFT | RIGHT | TOP | BOTTOM,
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
                    // .y = font_rasterizer.line_height + border_thickness,
                    .y = font_rasterizer.line_height,
                },
            .bg_color = Rgba::fromRgb(bg_color, 255),
            .bg_border_color = Rgba::fromRgb(colors::red, 0),
            .border_flags = border_flags,
        });
    };

    size_t selections_size = selections.size();
    for (size_t i = 0; i < selections_size; i++) {
        uint32_t middle_flags = 0;

        int start = selections[i].start;
        int end = selections[i].end;

        int curr_start = selections[i].start;
        int prev_start = i > 0 ? selections[i - 1].start : std::numeric_limits<int>::min();
        int next_start =
            i + 1 < selections_size ? selections[i + 1].start : std::numeric_limits<int>::min();

        if (curr_start < prev_start && curr_start < next_start) {
            if (prev_start < next_start) {
                start = next_start;
                create(curr_start, prev_start, selections[i].line,
                       LEFT | BOTTOM | TOP | BOTTOM_LEFT_INWARDS | TOP_LEFT_INWARDS);
                create(prev_start, next_start, selections[i].line, BOTTOM);
            } else {
                start = prev_start;
                create(curr_start, next_start, selections[i].line,
                       LEFT | BOTTOM | TOP | BOTTOM_LEFT_INWARDS | TOP_LEFT_INWARDS);
                create(next_start, prev_start, selections[i].line, TOP);
            }
        } else if (curr_start < prev_start) {
            start = prev_start;
            create(curr_start, prev_start, selections[i].line,
                   LEFT | TOP | BOTTOM_LEFT_OUTWARDS | TOP_LEFT_INWARDS);
        } else if (curr_start < next_start) {
            start = next_start;
            create(curr_start, next_start, selections[i].line,
                   LEFT | BOTTOM | BOTTOM_LEFT_INWARDS);
        } else {
            middle_flags |= LEFT;
            if (i > 0) middle_flags |= TOP_LEFT_OUTWARDS;
            if (i < selections_size - 1) middle_flags |= BOTTOM_LEFT_OUTWARDS;
        }

        int curr_end = selections[i].end;
        int prev_end = i > 0 ? selections[i - 1].end : std::numeric_limits<int>::max();
        int next_end =
            i + 1 < selections_size ? selections[i + 1].end : std::numeric_limits<int>::max();

        if (prev_end < curr_end && next_end < curr_end) {
            if (prev_end < next_end) {
                end = prev_end;
                create(prev_end, next_end, selections[i].line, TOP);
                create(next_end, curr_end, selections[i].line,
                       RIGHT | BOTTOM | TOP | BOTTOM_RIGHT_INWARDS | TOP_RIGHT_INWARDS);
            } else {
                end = next_end;
                create(next_end, prev_end, selections[i].line, BOTTOM);
                create(prev_end, next_end, selections[i].line,
                       RIGHT | BOTTOM | TOP | BOTTOM_RIGHT_INWARDS | TOP_RIGHT_INWARDS);
            }
        } else if (prev_end < curr_end) {
            end = prev_end;
            create(prev_end, curr_end, selections[i].line, RIGHT | TOP | TOP_RIGHT_INWARDS);
        } else if (next_end < curr_end) {
            end = next_end;
            create(next_end, curr_end, selections[i].line, RIGHT | BOTTOM | BOTTOM_RIGHT_INWARDS);
        } else {
            middle_flags |= RIGHT;
            if (i > 0) middle_flags |= TOP_RIGHT_OUTWARDS;
            if (i < selections_size - 1) middle_flags |= BOTTOM_RIGHT_OUTWARDS;
        }

        create(start, end, selections[i].line, middle_flags, Rgb{200, 200, 200});
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
