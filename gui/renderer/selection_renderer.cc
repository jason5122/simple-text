#include "selection_renderer.h"
#include <cassert>
#include <cstdint>

#include "opengl/gl.h"
using namespace opengl;

namespace {
const std::string kVertexShaderSource =
#include "gui/renderer/shaders/selection_vert.glsl"
    ;
const std::string kFragmentShaderSource =
#include "gui/renderer/shaders/selection_frag.glsl"
    ;
}

namespace gui {

SelectionRenderer::SelectionRenderer(GlyphCache& main_glyph_cache)
    : shader_program{kVertexShaderSource, kFragmentShaderSource},
      main_glyph_cache{main_glyph_cache} {
    instances.reserve(kBatchMax);

    GLuint shader_id = shader_program.id();
    glUseProgram(shader_id);
    glUniform1i(glGetUniformLocation(shader_id, "r"), kCornerRadius);
    glUniform1i(glGetUniformLocation(shader_id, "thickness"), kBorderThickness);

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
                          (void*)offsetof(InstanceData, size));
    glVertexAttribDivisor(index++, 1);

    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(InstanceData),
                          (void*)offsetof(InstanceData, color));
    glVertexAttribDivisor(index++, 1);

    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(InstanceData),
                          (void*)offsetof(InstanceData, border_color));
    glVertexAttribDivisor(index++, 1);

    // To prevent OpenGL from casting our ints to floats, we need to use either GL_FLOAT or
    // glVertexAttribIPointer().
    // https://stackoverflow.com/a/55972160
    glEnableVertexAttribArray(index);
    glVertexAttribIPointer(index, 4, GL_INT, sizeof(InstanceData),
                           (void*)offsetof(InstanceData, border_info));
    glVertexAttribDivisor(index++, 1);

    // Unbind.
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

SelectionRenderer::~SelectionRenderer() {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo_instance);
    glDeleteBuffers(1, &ebo);
}

SelectionRenderer::SelectionRenderer(SelectionRenderer&& other)
    : vao{other.vao},
      vbo_instance{other.vbo_instance},
      ebo{other.ebo},
      shader_program{std::move(other.shader_program)},
      main_glyph_cache{other.main_glyph_cache} {
    instances.reserve(kBatchMax);
    other.vao = 0;
    other.vbo_instance = 0;
    other.ebo = 0;
}

SelectionRenderer& SelectionRenderer::operator=(SelectionRenderer&& other) {
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

void SelectionRenderer::renderSelections(
    const Point& offset,
    const LineLayout& line_layout,
    std::vector<LineLayout::Token>::const_iterator start_caret,
    std::vector<LineLayout::Token>::const_iterator end_caret) {
    auto create = [&, this](int start, int end, int line,
                            uint32_t border_flags = kLeft | kRight | kTop | kBottom,
                            uint32_t bottom_border_offset = 0, uint32_t top_border_offset = 0,
                            uint32_t hide_background = 0) {
        instances.emplace_back(InstanceData{
            .coords =
                {
                    .x = static_cast<float>(start) + offset.x,
                    .y = static_cast<float>(main_glyph_cache.lineHeight() * line) + offset.y,
                },
            .size =
                {
                    .x = static_cast<float>(end - start),
                    .y = static_cast<float>(main_glyph_cache.lineHeight() + kBorderThickness),
                },
            .color = Rgba::fromRgb({227, 230, 232}, 0),
            .border_color = Rgba::fromRgb({212, 217, 221}, 0),
            // .border_color = Rgba::fromRgb(base::colors::red, 0),
            // .color = Rgba::fromRgb(base::colors::yellow, 0),
            // .border_color = Rgba::fromRgb(base::Rgb{0, 0, 0}, 0),
            .border_info =
                IVec4{
                    .x = border_flags,
                    .y = bottom_border_offset,
                    .z = top_border_offset,
                    .w = hide_background,
                },
        });
    };

    std::vector<SelectionRenderer::Selection> selections =
        getSelections(line_layout, start_caret, end_caret);

    size_t selections_size = selections.size();
    for (size_t i = 0; i < selections_size; i++) {
        uint32_t flags = kLeft | kRight;
        uint32_t bottom_border_offset = 0;
        uint32_t top_border_offset = 0;

        if (i == 0) {
            flags |= kTop | kTopLeftInwards | kTopRightInwards;

            if (selections_size > 1 && selections[i].start > 0) {
                int end = selections[i].start;
                if (selections[i + 1].end >= selections[i].start) {
                    end -= kCornerRadius + 2;
                }
                create(2, end, selections[i].line, kBottom, 0, 0, 1);
            }
        }
        if (i == selections_size - 1) {
            flags |= kBottom | kBottomLeftInwards | kBottomRightInwards;
        }

        if (i > 0) {
            if (selections[i - 1].start > 0) {
                flags |= kTopLeftInwards;
            }

            if (selections[i].end > selections[i - 1].end) {
                flags |= kTopRightInwards;

                flags |= kTop;
                top_border_offset = selections[i - 1].end;
            } else if (selections[i].end < selections[i - 1].end) {
                flags |= kTopRightOutwards;
            }
        }
        if (i + 1 < selections_size) {
            if (selections[i].start > selections[i + 1].start) {
                flags |= kBottomLeftOutwards;
            }

            if (selections[i].end > selections[i + 1].end) {
                flags |= kBottomRightInwards;

                flags |= kBottom;
                bottom_border_offset = selections[i + 1].end - selections[i].start;
            } else if (selections[i].end < selections[i + 1].end) {
                flags |= kBottomRightOutwards;
            }
        }

        create(selections[i].start, selections[i].end, selections[i].line, flags,
               bottom_border_offset, top_border_offset);
    }
}

void SelectionRenderer::render(const Size& screen_size, int rendering_pass) {
    glBlendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);

    GLuint shader_id = shader_program.id();
    glUseProgram(shader_id);
    glUniform2f(glGetUniformLocation(shader_id, "resolution"), screen_size.width,
                screen_size.height);
    glUniform1i(glGetUniformLocation(shader_id, "rendering_pass"), rendering_pass);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_instance);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(InstanceData) * instances.size(), &instances[0]);
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, instances.size());

    // Unbind.
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void SelectionRenderer::destroyInstances() {
    instances.clear();
}

std::vector<SelectionRenderer::Selection> SelectionRenderer::getSelections(
    const LineLayout& line_layout,
    std::vector<LineLayout::Token>::const_iterator start_caret,
    std::vector<LineLayout::Token>::const_iterator end_caret) {
    if (start_caret > end_caret) {
        std::swap(start_caret, end_caret);
    }
    assert(start_caret <= end_caret);

    std::vector<SelectionRenderer::Selection> selections;

    size_t start_line = (*start_caret).line;
    size_t end_line = (*end_caret).line;

    auto it = start_caret;
    while (it < end_caret) {
        size_t line = (*it).line;

        // Find either the next line break or the end caret, whichever comes first.
        auto next_it = std::prev(line_layout.getLine(line + 1));
        if (next_it >= end_caret) {
            next_it = std::prev(end_caret);
        }

        // Only render selection if
        // 1) the selection is visible, and
        // 2) the selection width is non-zero.
        int advance = (*it).total_advance;
        int next_advance = (*next_it).total_advance + (*next_it).advance;
        if ((start_line <= line && line <= end_line) && (advance != next_advance)) {
            selections.emplace_back(SelectionRenderer::Selection{
                .line = static_cast<int>((*it).line),
                .start = advance,
                .end = next_advance,
            });
        }

        it = std::next(next_it);
    }
    return selections;
}

}