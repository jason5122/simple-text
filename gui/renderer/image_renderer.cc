#include "image_renderer.h"

#include "base/filesystem/file_reader.h"

#include "opengl/gl.h"
using namespace opengl;

#include <cstring>
#include <png.h>
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
    loadPngNew(kPanelClose2xIndex, panel_close_2x);
    loadPngNew(kFolderOpen2xIndex, folder_open_2x);
    loadPngNew(kStanfordBunny, stanford_bunny);
    loadPngNew(kDice, dice);
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

size_t ImageRenderer::addPng(std::string_view image_path) {
    Image image;
    bool success = loadPng(image_path, image);
    // TODO: Handle image load failure in a more robust way.
    if (!success) {
        std::println("ImageRenderer::addPng() error: Could not load image.");
    }

    size_t image_id = cache.size();
    cache.emplace_back(std::move(image));
    return image_id;
}

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

bool ImageRenderer::loadPng(std::string_view file_name, Image& image) {
    std::unique_ptr<FILE, int (*)(FILE*)> fp{fopen(file_name.data(), "rb"), fclose};
    if (!fp) {
        return false;
    }

    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png_ptr) {
        return false;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_read_struct(&png_ptr, nullptr, nullptr);
        return false;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
        return false;
    }

    png_init_io(png_ptr, fp.get());

    int transforms =
        PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_PACKING | PNG_TRANSFORM_EXPAND | PNG_TRANSFORM_BGR;
    png_read_png(png_ptr, info_ptr, transforms, nullptr);

    png_uint_32 width, height;
    int bit_depth, color_type, interlace_type;
    png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type,
                 nullptr, nullptr);

    size_t row_bytes = png_get_rowbytes(png_ptr, info_ptr);
    png_bytepp row_pointers = png_get_rows(png_ptr, info_ptr);

    std::vector<uint8_t> buffer(row_bytes * height);
    for (png_uint_32 row = 0; row < height; ++row) {
        for (size_t col = 0; col < row_bytes; ++col) {
            // PNG is ordered top to bottom, but OpenGL expects bottom to top. This is fine!
            // We want to load the image flipped, since we flip y-coordinates in the vertex shader.
            buffer[row_bytes * row + col] = row_pointers[row][col];
        }
    }

    bool has_alpha = color_type & PNG_COLOR_MASK_ALPHA;
    Vec4 uv;
    atlas.insertTexture(width, height, has_alpha, buffer, uv);
    image = {
        .width = width,
        .height = height,
        .uv = uv,
    };

    png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
    return true;
}

bool ImageRenderer::loadPngNew(size_t index, std::string_view file_name) {
    PROFILE_BLOCK("ImageRenderer::loadPngNew()");

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

bool ImageRenderer::loadPng(size_t index, std::string_view file_name) {
    PROFILE_BLOCK("ImageRenderer::loadPng()");

    std::unique_ptr<FILE, int (*)(FILE*)> fp{fopen(file_name.data(), "rb"), fclose};
    if (!fp) {
        return false;
    }

    // Check if the file really is a PNG image.
    unsigned char sig[8];
    fread(sig, 1, 8, fp.get());
    if (!png_check_sig(sig, 8)) {
        return false;
    }

    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png_ptr) {
        return false;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_read_struct(&png_ptr, nullptr, nullptr);
        return false;
    }

    // setjmp() must be called in every function that calls a PNG-reading libpng function.
    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
        return false;
    }

    png_init_io(png_ptr, fp.get());
    png_set_sig_bytes(png_ptr, 8);  // We already read the 8 signature bytes.

    // Read all PNG info up to image data.
    png_read_info(png_ptr, info_ptr);
    png_uint_32 width = png_get_image_width(png_ptr, info_ptr);
    png_uint_32 height = png_get_image_height(png_ptr, info_ptr);
    png_byte color_type = png_get_color_type(png_ptr, info_ptr);
    size_t row_bytes = png_get_rowbytes(png_ptr, info_ptr);

    std::vector<uint8_t> buffer(row_bytes * height);

    // PNG is ordered top to bottom, but OpenGL expects bottom to top. This is fine!
    // We want to load the image flipped, since we flip y-coordinates in the vertex shader.
    std::vector<png_bytep> row_pointers(height);
    for (size_t i = 0; i < height; ++i) {
        row_pointers[i] = buffer.data() + i * row_bytes;
    }

    png_set_strip_16(png_ptr);
    png_set_bgr(png_ptr);
    png_set_packing(png_ptr);
    png_set_expand(png_ptr);

    png_read_image(png_ptr, row_pointers.data());

    bool has_alpha = color_type & PNG_COLOR_MASK_ALPHA;
    Vec4 uv;
    atlas.insertTexture(width, height, has_alpha, buffer, uv);
    cache[index] = {
        .width = width,
        .height = height,
        .uv = uv,
    };

    png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
    return true;
}

}  // namespace gui
