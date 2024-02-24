#include "image_renderer.h"
#include "ui/renderer/opengl_types.h"
#include "util/opengl_error_util.h"
#include <png.h>
#include <vector>

namespace {
struct InstanceData {
    Vec2 coords;
    Vec2 rect_size;
};
}

std::vector<uint8_t> read_png(fs::path file_name) {
    FILE* fp = fopen(file_name.c_str(), "rb");
    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop info_ptr = png_create_info_struct(png_ptr);
    png_init_io(png_ptr, fp);
    png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
    png_bytepp row_pointers = png_get_rows(png_ptr, info_ptr);

    png_uint_32 width = png_get_image_width(png_ptr, info_ptr);
    png_uint_32 height = png_get_image_height(png_ptr, info_ptr);
    std::cerr << width << "x" << height << '\n';

    std::vector<uint8_t> buffer;
    buffer.reserve(width * height);

    for (size_t y = 0; y < height; y++) {
        png_bytep row = row_pointers[y];
        for (size_t x = 0; x < width; x++) {
            png_bytep px = &(row[x * 4]);
            buffer.push_back(px[0]);
            buffer.push_back(px[1]);
            buffer.push_back(px[2]);
            buffer.push_back(px[3]);
        }
    }

    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);  // <-- destroying the info_ptr (as well).
    fclose(fp);

    return buffer;
}

void ImageRenderer::setup(float width, float height) {
    shader_program.link(ResourcePath() / "shaders/image_vert.glsl",
                        ResourcePath() / "shaders/image_frag.glsl");
    this->resize(width, height);

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
                          (void*)offsetof(InstanceData, rect_size));
    glVertexAttribDivisor(index++, 1);

    // Unbind.
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, &tex_id);
    glBindTexture(GL_TEXTURE_2D, tex_id);

    std::vector<uint8_t> buffer = read_png(ResourcePath() / "icons/wall.png");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 512, 512, 0, GL_RGBA, GL_UNSIGNED_BYTE, &buffer[0]);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);  // Unbind.
}

void ImageRenderer::draw(float scroll_x, float scroll_y) {
    glUseProgram(shader_program.id);
    glUniform2f(glGetUniformLocation(shader_program.id, "scroll_offset"), scroll_x, scroll_y);

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vao);

    std::vector<InstanceData> instances;

    instances.push_back(InstanceData{
        .coords = Vec2{width / 2, 400},
        .rect_size = Vec2{512, 512},
    });

    glBindBuffer(GL_ARRAY_BUFFER, vbo_instance);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(InstanceData) * instances.size(), &instances[0]);

    glBindTexture(GL_TEXTURE_2D, tex_id);

    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, instances.size());

    // Unbind.
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glCheckError();
}

void ImageRenderer::resize(float new_width, float new_height) {
    width = new_width;
    height = new_height;

    glViewport(0, 0, width, height);
    glUseProgram(shader_program.id);
    glUniform2f(glGetUniformLocation(shader_program.id, "resolution"), width, height);
}

ImageRenderer::~ImageRenderer() {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo_instance);
    glDeleteBuffers(1, &ebo);
}
