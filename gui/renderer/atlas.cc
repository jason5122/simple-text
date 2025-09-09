#include "base/rand_util.h"
#include "gui/renderer/atlas.h"
#include <spdlog/spdlog.h>

using namespace gl;

namespace gui {

Atlas::Atlas() {
    constexpr bool kPrintMaxTextureSize = false;
    if constexpr (kPrintMaxTextureSize) {
        GLint max_texture_size;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
        spdlog::info("GL_MAX_TEXTURE_SIZE = {}", max_texture_size);
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, &tex_id);
    glBindTexture(GL_TEXTURE_2D, tex_id);

    const void* data = nullptr;

    // DEBUG: Color atlas background to spot incorrect shaders easier.
    // This helped with debugging the fractional pixel scrolling bug.
    //
    // Creating this vector is very slow, so disable when not debugging (even during debug builds).
    constexpr bool kDebugTextureBleeding = false;
    if constexpr (kDebugTextureBleeding) {
        uint8_t r = base::rand_int(0, 255);
        uint8_t g = base::rand_int(0, 255);
        uint8_t b = base::rand_int(0, 255);

        size_t pixels = kAtlasSize * kAtlasSize;
        atlas_background.resize(pixels * 4);
        for (size_t i = 0; i < pixels; ++i) {
            size_t offset = i * 4;
            atlas_background[offset] = r;
            atlas_background[offset + 1] = g;
            atlas_background[offset + 2] = b;
            atlas_background[offset + 3] = 255;
        }

        data = atlas_background.data();
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kAtlasSize, kAtlasSize, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindTexture(GL_TEXTURE_2D, 0);  // Unbind.
}

Atlas::~Atlas() noexcept { glDeleteTextures(1, &tex_id); }

Atlas::Atlas(Atlas&& other) noexcept : tex_id(other.tex_id) { other.tex_id = 0; }

Atlas& Atlas::operator=(Atlas&& other) noexcept {
    if (&other != this) {
        tex_id = other.tex_id;
        other.tex_id = 0;
    }
    return *this;
}

GLuint Atlas::tex() const { return tex_id; }

// TODO: Consider refactoring this.
namespace {

GLenum format_to_glenum(Atlas::Format format) {
    switch (format) {
    case Atlas::Format::kBGRA:
        return GL_BGRA;
    case Atlas::Format::kRGBA:
        return GL_RGBA;
    case Atlas::Format::kRGB:
        return GL_RGB;
    }
}

}  // namespace

bool Atlas::insert_texture(
    int width, int height, Format format, const std::vector<uint8_t>& data, Vec4& out_uv) {
    if (width > kAtlasSize || height > kAtlasSize) {
        spdlog::error("Glyph is too large.");
        return false;
    }

    // If there's not enough room in current row, advance to the next one.
    if (!room_in_row(width, height)) {
        if (!advance_row()) {
            spdlog::error("Atlas is full.");
            return false;
        }

        // If there's still not enough room, then atlas is full.
        if (!room_in_row(width, height)) {
            spdlog::error("Could not insert into atlas.");
            return false;
        }
    }

    // Load data into OpenGL.
    GLenum gl_format = format_to_glenum(format);

    glBindTexture(GL_TEXTURE_2D, tex_id);
    glTexSubImage2D(GL_TEXTURE_2D, 0, row_extent, row_baseline, width, height, gl_format,
                    GL_UNSIGNED_BYTE, data.data());
    glBindTexture(GL_TEXTURE_2D, 0);  // Unbind.

    // Generate UV coordinates.
    float uv_x = static_cast<float>(row_extent) / kAtlasSize;
    float uv_y = static_cast<float>(row_baseline) / kAtlasSize;
    float uv_width = static_cast<float>(width) / kAtlasSize;
    float uv_height = static_cast<float>(height) / kAtlasSize;

    // Update atlas state.
    row_extent += width;
    row_tallest = std::max(height, row_tallest);

    out_uv = {uv_x, uv_y, uv_width, uv_height};
    return true;
}

bool Atlas::room_in_row(int width, int height) {
    int next_extent = row_extent + width;
    bool enough_width = next_extent <= kAtlasSize;
    bool enough_height = height < (kAtlasSize - row_baseline);

    return enough_width && enough_height;
}

bool Atlas::advance_row() {
    int advance_to = row_baseline + row_tallest;
    if (kAtlasSize - advance_to <= 0) {
        return false;
    }

    row_baseline = advance_to;
    row_extent = 0;
    row_tallest = 0;
    return true;
}

}  // namespace gui
