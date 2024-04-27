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

namespace {
struct Selection {
    int line;
    int start;
    int end;
};
}

static InstanceData CreateInstance(int start, int end, int line, uint32_t border_flags,
                                   FontRasterizer& font_rasterizer, Rgb color) {
    int border_thickness = 2;
    return {
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
        .bg_color = Rgba::fromRgb(colors::selection_unfocused, 255),
        .bg_border_color = Rgba::fromRgb(color, 0),
        .border_flags = border_flags,
    };
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

    std::vector<Selection> selections = {
        {.line = 2, .start = 195, .end = 424},
        {.line = 3, .start = 19 * 4, .end = 1558},
        {.line = 4, .start = 0, .end = 1558 - 19 * 4},
        // {.line = 5, .start = 19 * 2, .end = 376},
        {.line = 5, .start = 19 * 6, .end = 376},
    };
    // std::vector<Selection> selections = {
    //     {.line = 2, .start = 195, .end = 424},
    //     {.line = 3, .start = 19 * 50, .end = 1558},
    //     {.line = 4, .start = 0, .end = 19 * 5},
    //     {.line = 5, .start = 19 * 2, .end = 376},
    // };

    size_t selections_size = selections.size();
    uint32_t border_flags = 0;

    border_flags = LEFT | RIGHT | TOP | BOTTOM_LEFT_OUTWARDS | BOTTOM_RIGHT_OUTWARDS |
                   TOP_LEFT_INWARDS | TOP_RIGHT_INWARDS;
    if (selections_size == 1) {
        border_flags |= BOTTOM;
    }

    auto create = [&instances,
                   &font_rasterizer](int start, int end, int line,
                                     uint32_t border_flags = LEFT | RIGHT | TOP | BOTTOM,
                                     Rgb color = colors::red) {
        instances.emplace_back(
            CreateInstance(start, end, line, border_flags, font_rasterizer, color));
    };

    create(selections[0].start, selections[0].end, selections[0].line, border_flags);

    for (size_t i = 1; i < selections_size - 1; i++) {
        int p1 = selections[i].start;
        int p2 = selections[i].start;
        int p3 = selections[i].end;
        int p4 = selections[i].end;

        // Left.
        // border_flags = LEFT | RIGHT | TOP | BOTTOM;
        // instances.emplace_back(CreateInstance(p1, p2, selections[i].line, border_flags,
        //                                       font_rasterizer, colors::red));

        // Middle.
        border_flags = LEFT | RIGHT | TOP | BOTTOM;
        // if (selections[i].start < selections[i - 1].start) {
        //     border_flags |= TOP_LEFT_INWARDS;
        // } else if (selections[i].start > selections[i - 1].start) {
        //     border_flags |= TOP_LEFT_OUTWARDS;
        // }
        // if (selections[i].end > selections[i - 1].end) {
        //     border_flags |= TOP_RIGHT_INWARDS;
        // } else if (selections[i].end < selections[i - 1].end) {
        //     border_flags |= TOP_RIGHT_OUTWARDS;
        // }
        // if (selections[i].start < selections[i + 1].start) {
        //     border_flags |= BOTTOM_LEFT_INWARDS;
        // } else if (selections[i].start > selections[i + 1].start) {
        //     border_flags |= BOTTOM_LEFT_OUTWARDS;
        // }
        // if (selections[i].end > selections[i + 1].end) {
        //     border_flags |= BOTTOM_RIGHT_INWARDS;
        // } else if (selections[i].end > selections[i + 1].end) {
        //     border_flags |= BOTTOM_RIGHT_OUTWARDS;
        // }

        int curr_start = selections[i].start;
        int prev_start = selections[i - 1].start;
        int next_start = selections[i + 1].start;

        int start = curr_start;
        int end = selections[i].end;

        if (curr_start < prev_start && curr_start < next_start) {
            if (prev_start < next_start) {
                start = next_start;
                create(curr_start, prev_start, selections[i].line);
                create(prev_start, next_start, selections[i].line);
            } else {
                start = prev_start;
                create(curr_start, next_start, selections[i].line);
                create(next_start, prev_start, selections[i].line);
            }
        }

        int curr_end = selections[i].end;
        int prev_end = selections[i - 1].end;
        int next_end = selections[i + 1].end;

        if (curr_end > prev_end) {
            end = prev_end;
            create(prev_end, curr_end, selections[i].line);
        } else if (curr_end > next_end) {
            end = next_end;
            create(next_end, curr_end, selections[i].line);
        }

        create(start, end, selections[i].line, LEFT | RIGHT);

        // instances.emplace_back(CreateInstance(p2, p3, selections[i].line, border_flags,
        //                                       font_rasterizer, colors::blue));

        // Right.
        // border_flags = LEFT | RIGHT | TOP | BOTTOM;
        // instances.emplace_back(CreateInstance(p3, p4, selections[i].line, border_flags,
        //                                       font_rasterizer, colors::green));
    }

    if (selections_size > 1) {
        border_flags = LEFT | RIGHT | BOTTOM | BOTTOM_LEFT_INWARDS | BOTTOM_RIGHT_INWARDS |
                       TOP_LEFT_OUTWARDS | TOP_RIGHT_OUTWARDS;
        instances.emplace_back(CreateInstance(
            selections[selections_size - 1].start, selections[selections_size - 1].end,
            selections[selections_size - 1].line, border_flags, font_rasterizer, colors::purple));
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
