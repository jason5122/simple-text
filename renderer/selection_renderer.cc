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
    int total_advance;
    int line;
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

    std::vector<Temp> temps = {{19, 0},   {255, 1}, {424, 2}, {1558, 3},
                               {1558, 4}, {376, 5}, {19, 6}};

    int tab_corner_radius = 6;
    int border_thickness = 2;

    for (size_t i = 0; i < temps.size(); i++) {
        int start_x = 0;

        uint32_t border_flags = RIGHT;
        if (i == 0) {
            border_flags |= TOP | TOP_RIGHT_INWARDS | TOP_LEFT_INWARDS;
        }
        if (i == temps.size() - 1) {
            border_flags |= BOTTOM | BOTTOM_RIGHT_INWARDS | BOTTOM_LEFT_INWARDS;
        }

        if (i > 0) {
            if (temps[i].total_advance > temps[i - 1].total_advance) {
                border_flags |= TOP_RIGHT_INWARDS | TOP;
                start_x = temps[i - 1].total_advance;
                start_x += tab_corner_radius;
            } else if (temps[i].total_advance < temps[i - 1].total_advance) {
                border_flags |= TOP_RIGHT_OUTWARDS;
            }
        }
        if (i < temps.size() - 1) {
            if (temps[i].total_advance > temps[i + 1].total_advance) {
                border_flags |= BOTTOM_RIGHT_INWARDS | BOTTOM;
                start_x = temps[i + 1].total_advance;
                start_x += tab_corner_radius + border_thickness;
            } else if (temps[i].total_advance < temps[i + 1].total_advance) {
                border_flags |= BOTTOM_RIGHT_OUTWARDS;
            }
        }

        Vec2 coords{static_cast<float>(start_x - tab_corner_radius),
                    font_rasterizer.line_height * temps[i].line};
        Vec2 bg_size{static_cast<float>(temps[i].total_advance - start_x) + tab_corner_radius * 2,
                     font_rasterizer.line_height + border_thickness};

        instances.insert(instances.begin(),
                         InstanceData{
                             .coords = coords,
                             .bg_size = bg_size,
                             .bg_color = Rgba::fromRgb(colors::selection_unfocused, 255),
                             .bg_border_color = Rgba::fromRgb(colors::red, 0),
                             .border_flags = border_flags,
                         });
    }

    // instances.push_back(InstanceData{
    //     .coords = Vec2{19 * 10 - tab_corner_radius, font_rasterizer.line_height * 3},
    //     .bg_size = Vec2{19 * 10 - tab_corner_radius, font_rasterizer.line_height + 2},
    //     .bg_color = Rgba::fromRgb(colors::selection_unfocused, 255),
    //     .bg_border_color = Rgba::fromRgb(colors::red, 0),
    //     .border_flags = LEFT | RIGHT | TOP | TOP_LEFT_INWARDS,
    // });

    // instances.push_back(InstanceData{
    //     .coords = Vec2{19 * 14 - tab_corner_radius, font_rasterizer.line_height * 3},
    //     .bg_size = Vec2{19 * 6 - tab_corner_radius, font_rasterizer.line_height + 2},
    //     .bg_color = Rgba::fromRgb(colors::selection_unfocused, 255),
    //     .bg_border_color = Rgba::fromRgb(colors::red, 0),
    //     .border_flags = BOTTOM | TOP | RIGHT | BOTTOM_RIGHT_INWARDS | TOP_RIGHT_INWARDS,
    // });

    // instances.push_back(InstanceData{
    //     .coords = Vec2{19 * 10 - tab_corner_radius, font_rasterizer.line_height * 4},
    //     .bg_size = Vec2{19 * 5 - tab_corner_radius, font_rasterizer.line_height + 2},
    //     .bg_color = Rgba::fromRgb(colors::selection_unfocused, 255),
    //     .bg_border_color = Rgba::fromRgb(colors::red, 0),
    //     .border_flags = LEFT | RIGHT | BOTTOM | BOTTOM_LEFT_INWARDS | TOP_RIGHT_OUTWARDS |
    //                     BOTTOM_RIGHT_INWARDS,
    // });

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
