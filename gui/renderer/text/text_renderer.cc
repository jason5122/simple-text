#include "text_renderer.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

#include "opengl/gl.h"
using namespace opengl;

namespace {
const std::string kVertexShaderSource =
#include "gui/renderer/shaders/text_vert.glsl"
    ;
const std::string kFragmentShaderSource =
#include "gui/renderer/shaders/text_frag.glsl"
    ;
}

// TODO: Debug; remove this.
#include "util/profile_util.h"
#include <format>
#include <iostream>

namespace gui {

TextRenderer::TextRenderer(GlyphCache& main_glyph_cache, GlyphCache& ui_glyph_cache)
    : shader_program{kVertexShaderSource, kFragmentShaderSource},
      main_glyph_cache{main_glyph_cache},
      ui_glyph_cache{ui_glyph_cache} {
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
    glVertexAttribPointer(index, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
                          (void*)offsetof(InstanceData, glyph));
    glVertexAttribDivisor(index++, 1);

    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
                          (void*)offsetof(InstanceData, uv));
    glVertexAttribDivisor(index++, 1);

    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(InstanceData),
                          (void*)offsetof(InstanceData, color));
    glVertexAttribDivisor(index++, 1);

    // Unbind.
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

TextRenderer::~TextRenderer() {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo_instance);
    glDeleteBuffers(1, &ebo);
}

TextRenderer::TextRenderer(TextRenderer&& other)
    : vao{other.vao},
      vbo_instance{other.vbo_instance},
      ebo{other.ebo},
      shader_program{std::move(other.shader_program)},
      main_glyph_cache{other.main_glyph_cache},
      ui_glyph_cache{other.ui_glyph_cache} {
    other.vao = 0;
    other.vbo_instance = 0;
    other.ebo = 0;
}

TextRenderer& TextRenderer::operator=(TextRenderer&& other) {
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

void TextRenderer::renderText(size_t start_line,
                              size_t end_line,
                              const Point& offset,
                              const LineLayout& line_layout) {
    for (auto it = line_layout.line(start_line); it != line_layout.line(end_line); it++) {
        const auto& token = *it;

        Point coords{
            .x = token.total_advance,
            .y = static_cast<int>(token.line) * lineHeight(),
        };
        coords += offset;

        const InstanceData instance{
            .coords = coords.toVec2(),
            .glyph = token.glyph.glyph,
            .uv = token.glyph.uv,
            .color = Rgba::fromRgb({51, 51, 51}, token.glyph.colored),
        };
        insertIntoBatch(token.glyph.page, std::move(instance), true);
    }

    // TODO: Incorporate this into the build system.
    constexpr bool kDebugAtlas = false;
    if (kDebugAtlas) {
        int atlas_x_offset = 0;
        for (size_t page = 0; page < main_glyph_cache.atlas_pages.size(); page++) {
            Point coords{
                .x = atlas_x_offset,
                .y = 200,
            };
            coords += offset;

            InstanceData instance{
                .coords = coords.toVec2(),
                .glyph = Vec4{0, 0, Atlas::kAtlasSize, Atlas::kAtlasSize},
                .uv = Vec4{0, 0, 1.0, 1.0},
                .color = Rgba{255, 0, 0, 0},
            };
            insertIntoBatch(page, std::move(instance), true);

            atlas_x_offset += Atlas::kAtlasSize + 100;
        }
    }
}

void TextRenderer::addUiText(const Point& coords, const Rgb& color, const base::Utf8String& str8) {
    int total_advance = 0;
    for (const auto& ch : str8.getChars()) {
        GlyphCache::Glyph& glyph = ui_glyph_cache.getGlyph(ch.str);

        // TODO: Rename this.
        Point pos{
            .x = total_advance,
            .y = static_cast<int>(0) * ui_glyph_cache.lineHeight(),
        };
        pos += coords;

        InstanceData instance{
            .coords = pos.toVec2(),
            .glyph = glyph.glyph,
            .uv = glyph.uv,
            .color = Rgba::fromRgb(color, glyph.colored),
        };
        insertIntoBatch(glyph.page, std::move(instance), false);

        total_advance += glyph.advance;
    }
}

void TextRenderer::flush(const Size& screen_size, bool use_main_glyph_cache) {
    auto& glyph_cache = use_main_glyph_cache ? main_glyph_cache : ui_glyph_cache;
    auto& batch_instances = use_main_glyph_cache ? main_batch_instances : ui_batch_instances;

    glBlendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);

    GLuint shader_id = shader_program.id();
    glUseProgram(shader_id);
    glUniform1f(glGetUniformLocation(shader_id, "line_height"), glyph_cache.lineHeight());
    glUniform2f(glGetUniformLocation(shader_id, "resolution"), screen_size.width,
                screen_size.height);

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vao);

    for (size_t page = 0; page < glyph_cache.atlas_pages.size(); page++) {
        while (batch_instances.size() <= page) {
            batch_instances.emplace_back();
            batch_instances.back().reserve(kBatchMax);
        }

        GLuint batch_tex = glyph_cache.atlas_pages.at(page).tex();
        std::vector<InstanceData>& instances = batch_instances.at(page);

        if (instances.empty()) {
            return;
        }

        glBindBuffer(GL_ARRAY_BUFFER, vbo_instance);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(InstanceData) * instances.size(),
                        &instances[0]);

        glBindTexture(GL_TEXTURE_2D, batch_tex);

        glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, instances.size());

        instances.clear();
    }

    // Unbind.
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

int TextRenderer::lineHeight() {
    return main_glyph_cache.lineHeight();
}

int TextRenderer::uiLineHeight() {
    return ui_glyph_cache.lineHeight();
}

void TextRenderer::insertIntoBatch(size_t page,
                                   const InstanceData& instance,
                                   bool use_main_glyph_cache) {
    auto& batch_instances = use_main_glyph_cache ? main_batch_instances : ui_batch_instances;

    while (batch_instances.size() <= page) {
        batch_instances.emplace_back();
        batch_instances.back().reserve(kBatchMax);
    }

    std::vector<InstanceData>& instances = batch_instances.at(page);

    instances.emplace_back(std::move(instance));
    if (instances.size() == kBatchMax) {
        std::cerr << "TextRenderer error: attempted to insert into a full batch!\n";
    }
}

}
