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
struct Temp {
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
                .y = font_rasterizer.line_height + border_thickness,
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

    std::vector<Temp> temps = {
        {.line = 2, .start = 19 * 10, .end = 424},
        {.line = 3, .start = 19 * 4, .end = 1558},
        {.line = 4, .end = 1558 + 19 * 4},
    };
    // std::vector<Temp> temps = {
    //     {.line = 3, .start = 19 * 50, .end = 1558},
    //     {.line = 4, .start = 0, .end = 19 * 5},
    // };

    uint32_t border_flags = 0;

    for (size_t i = 0; i < temps.size(); i++) {
        if (i > 0) {
            if (temps[i].start < temps[i - 1].start) {
                border_flags = LEFT | RIGHT | TOP | BOTTOM;
                instances.insert(instances.begin(),
                                 CreateInstance(temps[i].start, temps[i - 1].start, temps[i].line,
                                                border_flags, font_rasterizer, colors::red));
            }

            border_flags = LEFT | RIGHT | TOP | BOTTOM;
            instances.insert(instances.begin(),
                             CreateInstance(temps[i].start, temps[i - 1].end, temps[i].line,
                                            border_flags, font_rasterizer, colors::blue));

            if (temps[i].end >= temps[i - 1].end) {
                border_flags = LEFT | RIGHT | TOP | BOTTOM;
                instances.insert(instances.begin(),
                                 CreateInstance(temps[i - 1].end, temps[i].end, temps[i].line,
                                                border_flags, font_rasterizer, colors::green));
            }
        } else if (i == 0) {
            border_flags = LEFT | RIGHT | TOP | BOTTOM;
            instances.insert(instances.begin(),
                             CreateInstance(temps[i].start, temps[i].end, temps[i].line,
                                            border_flags, font_rasterizer, colors::purple));
        }
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
