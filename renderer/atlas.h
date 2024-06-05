#pragma once

#include "renderer/opengl_types.h"
#include "util/not_copyable_or_movable.h"
#include <glad/glad.h>
#include <vector>

namespace renderer {

class Atlas {
public:
    // https://feedback.wildfiregames.com/report/opengl/feature/GL_MAX_TEXTURE_SIZE
    static constexpr int kAtlasSize = 1024;  // 1024 is a conservative size.

    GLuint tex_id;

    NOT_COPYABLE(Atlas)
    NOT_MOVABLE(Atlas)
    Atlas() = default;
    ~Atlas();
    void setup();
    Vec4 insertTexture(int width, int height, bool colored, GLubyte* data);

private:
    int row_extent = 0;
    int row_baseline = 0;
    int row_tallest = 0;

    // DEBUG: Color atlas background to spot incorrect shaders easier.
    std::vector<uint8_t> atlas_background;

    bool roomInRow(int width, int height);
    bool advanceRow();
};

}
