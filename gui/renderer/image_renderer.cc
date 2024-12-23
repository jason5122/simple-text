#include "image_renderer.h"

#include "base/filesystem/file_reader.h"

#include "opengl/gl.h"
using namespace opengl;

#include <spng.h>

#include <jerror.h>
#include <jpeglib.h>

// TODO: Debug use; remove this.
#include "util/profile_util.h"
#include <fmt/base.h>
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

// TODO: De-duplicate this code in a clean way.
size_t ImageRenderer::addPng(std::string_view image_path) {
    Image image;
    bool success = loadPng(image_path, image);
    // TODO: Handle image load failure in a more robust way.
    if (!success) {
        fmt::println("ImageRenderer::addJpeg() error: Could not load image.");
    }

    size_t image_id = cache.size();
    cache.emplace_back(std::move(image));
    return image_id;
}

// TODO: De-duplicate this code in a clean way.
size_t ImageRenderer::addJpeg(std::string_view image_path) {
    Image image;
    bool success = loadJpeg(image_path, image);
    // TODO: Handle image load failure in a more robust way.
    if (!success) {
        fmt::println("ImageRenderer::addJpeg() error: Could not load image.");
    }

    size_t image_id = cache.size();
    cache.emplace_back(std::move(image));
    return image_id;
}

const ImageRenderer::Image& ImageRenderer::get(size_t image_id) const {
    return cache[image_id];
}

// TODO: Find a way to not have to cast here.
void ImageRenderer::insertInBatch(size_t image_index,
                                  const app::Point& coords,
                                  const Rgba& color) {
    const Image& image = cache.at(image_index);
    instances.emplace_back(InstanceData{
        .coords = {static_cast<float>(coords.x), static_cast<float>(coords.y)},
        .rect_size = {static_cast<float>(image.width), static_cast<float>(image.height)},
        .uv = image.uv,
        .color = color,
    });
}

void ImageRenderer::renderBatch(const app::Size& screen_size) {
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

// TODO: Handle errors.
bool ImageRenderer::loadPng(std::string_view file_name, Image& image) {
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

    Vec4 uv;
    atlas.insertTexture(width, height, Atlas::Format::kRGBA, buffer, uv);
    image = {
        .width = width,
        .height = height,
        .uv = uv,
    };
    return true;
}

// TODO: Handle errors.
bool ImageRenderer::loadJpeg(std::string_view file_name, Image& image) {
    PROFILE_BLOCK("ImageRenderer::loadJpeg()");

    jpeg_decompress_struct info;
    jpeg_error_mgr err;

    std::unique_ptr<FILE, int (*)(FILE*)> fp{fopen(file_name.data(), "rb"), fclose};
    if (!fp) {
        return false;
    }

    info.err = jpeg_std_error(&err);
    jpeg_create_decompress(&info);

    jpeg_stdio_src(&info, fp.get());
    jpeg_read_header(&info, TRUE);

    jpeg_start_decompress(&info);
    JDIMENSION width = info.output_width;
    JDIMENSION height = info.output_height;
    int num_components = info.num_components;
    size_t row_bytes = width * num_components;

    std::vector<uint8_t> buffer(width * height * num_components);
    uint8_t* row_buffer[1];
    while (info.output_scanline < height) {
        row_buffer[0] = &buffer[info.output_scanline * row_bytes];
        jpeg_read_scanlines(&info, row_buffer, 1);
    }

    jpeg_finish_decompress(&info);
    jpeg_destroy_decompress(&info);

    Vec4 uv;
    atlas.insertTexture(width, height, Atlas::Format::kRGB, buffer, uv);
    image = {
        .width = width,
        .height = height,
        .uv = uv,
    };
    return true;
}

}  // namespace gui
