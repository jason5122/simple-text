#include "base/filesystem/file_reader.h"
#include "image_renderer.h"
#include <cstring>
#include <png.h>

#include "opengl/gl.h"
using namespace opengl;

// TODO: Debug use; remove this.
#include <cassert>
#include <format>
#include <iostream>

namespace {
const std::string kVertexShaderSource =
#include "gui/renderer/shaders/image_vert.glsl"
    ;
const std::string kFragmentShaderSource =
#include "gui/renderer/shaders/image_frag.glsl"
    ;
}

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

    fs::path panel_close_2x = ResourceDir() / "icons/panel_close@2x.png";
    fs::path folder_open_2x = ResourceDir() / "icons/folder_open@2x.png";

    // TODO: Figure out a better way to do this.
    image_atlas_entries.resize(2);
    loadPng(kPanelClose2xIndex, panel_close_2x);
    loadPng(kFolderOpen2xIndex, folder_open_2x);
}

ImageRenderer::~ImageRenderer() {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo_instance);
    glDeleteBuffers(1, &ebo);
}

ImageRenderer::ImageRenderer(ImageRenderer&& other)
    : vao{other.vao},
      vbo_instance{other.vbo_instance},
      ebo{other.ebo},
      shader_program{std::move(other.shader_program)} {
    other.vao = 0;
    other.vbo_instance = 0;
    other.ebo = 0;
}

ImageRenderer& ImageRenderer::operator=(ImageRenderer&& other) {
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

// TODO: Store image size as Size (ints) instead of Vec2 (floats).
Size ImageRenderer::getImageSize(size_t image_index) {
    AtlasImage& atlas_entry = image_atlas_entries.at(image_index);
    return {
        .width = static_cast<int>(atlas_entry.rect_size.x),
        .height = static_cast<int>(atlas_entry.rect_size.y),
    };
}

void ImageRenderer::addImage(size_t image_index, const Point& coords, const Rgba& color) {
    AtlasImage& atlas_entry = image_atlas_entries.at(image_index);
    instances.push_back(InstanceData{
        .coords = Vec2{static_cast<float>(coords.x), static_cast<float>(coords.y)},
        .rect_size = atlas_entry.rect_size,
        .uv = atlas_entry.uv,
        .color = color,
    });
}

void ImageRenderer::flush(const Size& screen_size) {
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE);

    GLuint shader_id = shader_program.id();
    glUseProgram(shader_id);
    glUniform2f(glGetUniformLocation(shader_id, "resolution"), screen_size.width,
                screen_size.height);

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_instance);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(InstanceData) * instances.size(), &instances[0]);
    glBindTexture(GL_TEXTURE_2D, atlas.tex());
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, instances.size());

    // Unbind.
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    instances.clear();
}

bool ImageRenderer::loadPng(size_t index, fs::path file_name) {
    std::unique_ptr<FILE, int (*)(FILE*)> fp{fopen(file_name.string().c_str(), "rb"), fclose};

    if (!fp) {
        return false;
    }

    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (png_ptr == nullptr) {
        return false;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == nullptr) {
        png_destroy_read_struct(&png_ptr, nullptr, nullptr);
        return false;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
        return false;
    }

    png_init_io(png_ptr, fp.get());

    png_set_sig_bytes(png_ptr, 0);

    png_read_png(png_ptr, info_ptr,
                 PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_PACKING | PNG_TRANSFORM_EXPAND, nullptr);

    png_uint_32 width, height;
    int bit_depth, color_type, interlace_type;
    png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type,
                 nullptr, nullptr);

    size_t row_bytes = png_get_rowbytes(png_ptr, info_ptr);
    png_bytepp row_pointers = png_get_rows(png_ptr, info_ptr);

    std::vector<uint8_t> buffer(row_bytes * height);
    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < row_bytes; ++j) {
            // PNG is ordered top to bottom, but OpenGL expects bottom to top. This is fine!
            // We want to load the image flipped, since we flip y-coordinates in the vertex shader.
            // int row = height - 1 - i;

            int row = i;
            buffer[row_bytes * row + j] = row_pointers[i][j];
        }
    }

    Vec4 uv;
    atlas.insertTexture(width, height, color_type == PNG_COLOR_TYPE_RGBA, buffer, uv);
    image_atlas_entries[index] = {
        .rect_size = Vec2{static_cast<float>(width), static_cast<float>(height)},
        .uv = uv,
    };

    png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
    return true;
}

}
