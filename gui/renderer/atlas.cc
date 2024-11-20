#include "atlas.h"

using namespace opengl;

// TODO: For debugging; remove this.
#include "util/std_print.h"
#include <random>

namespace gui {

Atlas::Atlas() {
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, &tex_id);
    glBindTexture(GL_TEXTURE_2D, tex_id);

    const void* data = nullptr;

    // TODO: Incorporate this into the build system.
    constexpr bool kDebugAtlas = false;
    if constexpr (kDebugAtlas) {
        std::random_device dev;
        std::mt19937 rng(dev());
        std::uniform_int_distribution<std::mt19937::result_type> dist(0, 255);

        uint8_t r = dist(rng);
        uint8_t g = dist(rng);
        uint8_t b = dist(rng);

        // DEBUG: Color atlas background to spot incorrect shaders easier.
        // This helped with debugging the fractional pixel scrolling bug.
        // TODO: Creating this` vector is quite slow, so disable during release.
        atlas_background = std::vector<uint8_t>(kAtlasSize * kAtlasSize * 4);
        size_t pixels = kAtlasSize * kAtlasSize;
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

Atlas::~Atlas() {
    glDeleteTextures(1, &tex_id);
}

Atlas::Atlas(Atlas&& other) : tex_id(other.tex_id) {
    other.tex_id = 0;
}

Atlas& Atlas::operator=(Atlas&& other) {
    if (&other != this) {
        tex_id = other.tex_id;
        other.tex_id = 0;
    }
    return *this;
}

GLuint Atlas::tex() const {
    return tex_id;
}

bool Atlas::insertTexture(int width, int height, bool colored, const GLubyte* data, Vec4& out_uv) {
    if (width > kAtlasSize || height > kAtlasSize) {
        std::println("Glyph is too large.");
        return false;
    }

    // If there's not enough room in current row, advance to the next one.
    if (!roomInRow(width, height)) {
        if (!advanceRow()) {
            std::println("Atlas is full.");
            return false;
        }

        // If there's still not enough room, then atlas is full.
        if (!roomInRow(width, height)) {
            std::println("Could not insert into atlas.");
            return false;
        }
    }

    // Load data into OpenGL.
    glBindTexture(GL_TEXTURE_2D, tex_id);
    glTexSubImage2D(GL_TEXTURE_2D, 0, row_extent, row_baseline, width, height, GL_BGRA,
                    GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_2D, 0);  // Unbind.

    // Generate UV coordinates.
    float uv_left = static_cast<float>(row_extent) / kAtlasSize;
    float uv_bot = static_cast<float>(row_baseline) / kAtlasSize;
    float uv_width = static_cast<float>(width) / kAtlasSize;
    float uv_height = static_cast<float>(height) / kAtlasSize;

    // Update Atlas state.
    row_extent += width;
    row_tallest = std::max(height, row_tallest);

    out_uv = Vec4{uv_left, uv_bot, uv_width, uv_height};
    return true;
}

bool Atlas::roomInRow(int width, int height) {
    int next_extent = row_extent + width;
    bool enough_width = next_extent <= kAtlasSize;
    bool enough_height = height < (kAtlasSize - row_baseline);

    return enough_width && enough_height;
}

bool Atlas::advanceRow() {
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
