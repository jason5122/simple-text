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
#include "util/std_print.h"

namespace gui {

TextRenderer::TextRenderer(GlyphCache& glyph_cache)
    : glyph_cache{glyph_cache}, shader_program{kVertexShaderSource, kFragmentShaderSource} {
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
    : glyph_cache{other.glyph_cache},
      shader_program{std::move(other.shader_program)},
      vao{other.vao},
      vbo_instance{other.vbo_instance},
      ebo{other.ebo} {
    other.vao = 0;
    other.vbo_instance = 0;
    other.ebo = 0;
}

TextRenderer& TextRenderer::operator=(TextRenderer&& other) {
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

void TextRenderer::flush(const Size& screen_size, TextLayer font_type) {
    auto& batch_instances = font_type == TextLayer::kForeground ? foreground_batch_instances
                                                                : background_batch_instances;

    glBlendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);

    GLuint shader_id = shader_program.id();
    glUseProgram(shader_id);
    glUniform2f(glGetUniformLocation(shader_id, "resolution"), screen_size.width,
                screen_size.height);

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vao);

    for (size_t page = 0; page < glyph_cache.atlasPages().size(); ++page) {
        // TODO: Refactor this ugly hack.
        while (batch_instances.size() <= page) {
            batch_instances.emplace_back();
            batch_instances.back().reserve(kBatchMax);
        }

        GLuint batch_tex = glyph_cache.atlasPages().at(page).tex();
        std::vector<InstanceData>& instances = batch_instances.at(page);

        if (instances.empty()) {
            return;
        }

        glBindBuffer(GL_ARRAY_BUFFER, vbo_instance);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(InstanceData) * instances.size(),
                        instances.data());

        glBindTexture(GL_TEXTURE_2D, batch_tex);

        glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, instances.size());

        instances.clear();
    }

    // Unbind.
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void TextRenderer::renderAtlasPages(const Point& coords) {
    int atlas_x_offset = 0;
    for (size_t page = 0; page < glyph_cache.atlasPages().size(); ++page) {
        Point atlas_coords{
            .x = atlas_x_offset,
            .y = 0,
        };
        atlas_coords += coords;

        InstanceData instance{
            .coords = atlas_coords.toVec2(),
            .glyph = Vec4{0, 0, Atlas::kAtlasSize, Atlas::kAtlasSize},
            .uv = Vec4{0, 0, 1.0, 1.0},
            .color = Rgba{255, 255, 255, true},
        };
        insertIntoBatch(page, std::move(instance), TextLayer::kForeground);

        atlas_x_offset += Atlas::kAtlasSize + 100;
    }
}

void TextRenderer::insertIntoBatch(size_t page,
                                   const InstanceData& instance,
                                   TextLayer font_type) {
    auto& batch_instances = font_type == TextLayer::kForeground ? foreground_batch_instances
                                                                : background_batch_instances;

    // TODO: Refactor this ugly hack.
    while (batch_instances.size() <= page) {
        batch_instances.emplace_back();
        batch_instances.back().reserve(kBatchMax);
    }

    std::vector<InstanceData>& instances = batch_instances.at(page);

    instances.emplace_back(std::move(instance));
    if (instances.size() == kBatchMax) {
        std::println("TextRenderer error: attempted to insert into a full batch!");
    }
}

}
