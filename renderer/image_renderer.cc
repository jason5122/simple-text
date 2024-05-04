#include "image_renderer.h"
#include "renderer/opengl_error_util.h"
#include "renderer/opengl_types.h"
#include <cstring>
#include <png.h>

namespace renderer {
namespace {
struct InstanceData {
    Vec2 coords;
    Vec2 rect_size;
    Vec4 uv;
    Vec3 color;
};
}

ImageRenderer::ImageRenderer() {}

ImageRenderer::~ImageRenderer() {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo_instance);
    glDeleteBuffers(1, &ebo);
}

void ImageRenderer::setup() {
    std::string vert_source =
#include "shaders/image_vert.glsl"
        ;
    std::string frag_source =
#include "shaders/image_frag.glsl"
        ;

    shader_program.link(vert_source, frag_source);
    atlas.setup(false);  // Disable bilinear filtering.

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
                          (void*)offsetof(InstanceData, rect_size));
    glVertexAttribDivisor(index++, 1);

    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
                          (void*)offsetof(InstanceData, uv));
    glVertexAttribDivisor(index++, 1);

    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, 3, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
                          (void*)offsetof(InstanceData, color));
    glVertexAttribDivisor(index++, 1);

    // Unbind.
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    fs::path image_path = ResourceDir() / "icons/panel_close@2x.png";
    // fs::path image_path = ResourceDir() / "icons/folder_open@2x.png";

    int out_width, out_height;
    bool out_has_alpha;
    GLubyte* out_data;
    this->loadPng(image_path, out_width, out_height, out_has_alpha, &out_data);

    Vec4 uv = atlas.insertTexture(out_width, out_height, out_has_alpha, out_data);
    image_atlas_entries.push_back(AtlasImage{
        .rect_size = Vec2{static_cast<float>(out_width), static_cast<float>(out_height)},
        .uv = uv,
    });
}

void ImageRenderer::draw(Size& size, Point& scroll, Point& editor_offset,
                         std::vector<int>& tab_title_x_coords,
                         std::vector<int>& actual_tab_title_widths) {
    glUseProgram(shader_program.id);
    glUniform2f(glGetUniformLocation(shader_program.id, "resolution"), size.width, size.height);
    glUniform2f(glGetUniformLocation(shader_program.id, "scroll_offset"), scroll.x, scroll.y);

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vao);

    std::vector<InstanceData> instances;

    AtlasImage atlas_entry = image_atlas_entries[0];
    float tab_width = 350;
    float tab_offset_from_top = 5;
    float tab_corner_radius = 10;
    float close_button_offset = 10;

    for (size_t i = 0; i < tab_title_x_coords.size(); i++) {
        // float pos_x = (editor_offset.x - close_button_offset) +
        //               ((tab_width - tab_corner_radius) - atlas_entry.rect_size.x);
        float pos_y = size.height - (atlas_entry.rect_size.y / 2) -
                      ((editor_offset.y + tab_offset_from_top) / 2);

        instances.push_back(InstanceData{
            .coords =
                Vec2{
                    .x = editor_offset.x +
                         static_cast<float>(tab_title_x_coords[i] + (actual_tab_title_widths[i] -
                                                                     atlas_entry.rect_size.x)),
                    .y = pos_y,
                },
            // .coords = Vec2{pos_x, pos_y},
            .rect_size = atlas_entry.rect_size,
            .uv = atlas_entry.uv,
            .color = Vec3{158, 158, 158},
            // .color = Vec3{116, 116, 116},
        });
    }

    glBindBuffer(GL_ARRAY_BUFFER, vbo_instance);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(InstanceData) * instances.size(), &instances[0]);

    glBindTexture(GL_TEXTURE_2D, atlas.tex_id);

    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, instances.size());

    // Unbind.
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glCheckError();
}

// https://blog.nobel-joergensen.com/2010/11/07/loading-a-png-as-texture-in-opengl-using-libpng/
bool ImageRenderer::loadPng(fs::path file_name, int& out_width, int& out_height,
                            bool& out_has_alpha, GLubyte** out_data) {
    png_structp png_ptr;
    png_infop info_ptr;
    unsigned int sig_read = 0;
    int color_type, interlace_type;
    FILE* fp;

    if ((fp = fopen(file_name.string().c_str(), "rb")) == NULL) {
        return false;
    }

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (png_ptr == NULL) {
        fclose(fp);
        return false;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL) {
        fclose(fp);
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        return false;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        return false;
    }

    png_init_io(png_ptr, fp);

    png_set_sig_bytes(png_ptr, sig_read);

    png_read_png(png_ptr, info_ptr,
                 PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_PACKING | PNG_TRANSFORM_EXPAND, NULL);

    png_uint_32 width, height;
    int bit_depth;
    png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type,
                 NULL, NULL);
    out_width = width;
    out_height = height;
    out_has_alpha = color_type == PNG_COLOR_TYPE_RGBA;

    unsigned int row_bytes = png_get_rowbytes(png_ptr, info_ptr);
    *out_data = (unsigned char*)malloc(row_bytes * out_height);

    png_bytepp row_pointers = png_get_rows(png_ptr, info_ptr);

    for (int i = 0; i < out_height; i++) {
        // PNG is ordered top to bottom, but OpenGL expects bottom to top.
        memcpy(*out_data + (row_bytes * (out_height - 1 - i)), row_pointers[i], row_bytes);
    }

    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    fclose(fp);

    return true;
}
}
