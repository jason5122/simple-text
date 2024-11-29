#include "image_renderer.h"

#include "base/filesystem/file_reader.h"

#include "opengl/gl.h"
using namespace opengl;

#include <cstring>
#include <spng.h>

// TODO: Debug use; remove this.
#include "util/profile_util.h"
#include "util/std_print.h"
#include <cassert>

namespace {

const std::string kVertexShaderSource =
#include "gui/renderer/shaders/image_vert.glsl"
    ;
const std::string kFragmentShaderSource =
#include "gui/renderer/shaders/image_frag.glsl"
    ;

}  // namespace

namespace gui {

ImageRenderer::ImageRenderer() : shader_program{kVertexShaderSource, kFragmentShaderSource} {
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

    std::string panel_close_2x = std::format("{}/icons/panel_close@2x.png", base::ResourceDir());
    std::string folder_open_2x = std::format("{}/icons/folder_open@2x.png", base::ResourceDir());
    std::string stanford_bunny = std::format("{}/icons/stanford_bunny.png", base::ResourceDir());
    std::string dice = std::format("{}/icons/dice.png", base::ResourceDir());

    // TODO: Figure out a better way to do this.
    cache.resize(4);
    loadPng(kPanelClose2xIndex, panel_close_2x);
    loadPng(kFolderOpen2xIndex, folder_open_2x);
    loadPng(kStanfordBunny, stanford_bunny);
    loadPng(kDice, dice);
}

ImageRenderer::~ImageRenderer() {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo_instance);
    glDeleteBuffers(1, &ebo);
}

ImageRenderer::ImageRenderer(ImageRenderer&& other)
    : shader_program{std::move(other.shader_program)},
      vao{other.vao},
      vbo_instance{other.vbo_instance},
      ebo{other.ebo} {
    other.vao = 0;
    other.vbo_instance = 0;
    other.ebo = 0;
}

ImageRenderer& ImageRenderer::operator=(ImageRenderer&& other) {
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

// size_t ImageRenderer::addPng(std::string_view image_path) {
//     Image image;
//     bool success = loadPng(image_path, image);
//     // TODO: Handle image load failure in a more robust way.
//     if (!success) {
//         std::println("ImageRenderer::addPng() error: Could not load image.");
//     }

//     size_t image_id = cache.size();
//     cache.emplace_back(std::move(image));
//     return image_id;
// }

const ImageRenderer::Image& ImageRenderer::get(size_t image_id) const {
    return cache[image_id];
}

// TODO: Find a way to not have to cast here.
app::Size ImageRenderer::getImageSize(size_t image_index) {
    const Image& image = cache.at(image_index);
    return {
        .width = static_cast<int>(image.width),
        .height = static_cast<int>(image.height),
    };
}

// TODO: Find a way to not have to cast here.
void ImageRenderer::addImage(size_t image_index, const app::Point& coords, const Rgba& color) {
    const Image& image = cache.at(image_index);
    instances.emplace_back(InstanceData{
        .coords = {static_cast<float>(coords.x), static_cast<float>(coords.y)},
        .rect_size = {static_cast<float>(image.width), static_cast<float>(image.height)},
        .uv = image.uv,
        .color = color,
    });
}

void ImageRenderer::flush(const app::Size& screen_size) {
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE);

    GLuint shader_id = shader_program.id();
    glUseProgram(shader_id);
    glUniform2f(glGetUniformLocation(shader_id, "resolution"), screen_size.width,
                screen_size.height);

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_instance);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(InstanceData) * instances.size(), instances.data());
    glBindTexture(GL_TEXTURE_2D, atlas.tex());
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, instances.size());

    // Unbind.
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    instances.clear();
}

bool ImageRenderer::loadPng(size_t index, std::string_view file_name) {
    PROFILE_BLOCK("ImageRenderer::loadPng()");

    std::unique_ptr<FILE, int (*)(FILE*)> fp{fopen(file_name.data(), "rb"), fclose};
    std::unique_ptr<spng_ctx, void (*)(spng_ctx*)> ctx{spng_ctx_new(0), spng_ctx_free};
    spng_ihdr ihdr;
    size_t size;

    if (!fp) return false;
    if (!ctx) return false;

    if (spng_set_png_file(ctx.get(), fp.get())) return false;

    if (spng_get_ihdr(ctx.get(), &ihdr)) return false;

    if (spng_decoded_image_size(ctx.get(), SPNG_FMT_RGBA8, &size)) return false;

    std::vector<uint8_t> buffer(size);
    if (spng_decode_image(ctx.get(), buffer.data(), size, SPNG_FMT_RGBA8, SPNG_DECODE_TRNS)) {
        return false;
    }

    uint32_t width = ihdr.width;
    uint32_t height = ihdr.height;

    // TODO: Accept the RGBA format in Atlas.
    Vec4 uv;
    atlas.insertTexture(width, height, true, buffer, uv);
    cache[index] = {
        .width = width,
        .height = height,
        .uv = uv,
    };
    return true;
}

}  // namespace gui
