#pragma once

#include "renderer/opengl_types.h"
#include "util/not_copyable_or_movable.h"
#include <glad/glad.h>
#include <vector>

namespace renderer {

class Atlas {
public:
    // 1024 is a conservative size.
    // https://feedback.wildfiregames.com/report/opengl/feature/GL_MAX_TEXTURE_SIZE
    // static constexpr int kAtlasSize = 1024;
    static constexpr int kAtlasSize = 512;

    NOT_COPYABLE(Atlas)
    Atlas();
    ~Atlas();
    Atlas(Atlas&&);
    Atlas& operator=(Atlas&&);

    void setup();
    GLuint tex() const;
    bool insertTexture(int width, int height, bool colored, const GLubyte* data, Vec4& uv);

private:
    GLuint tex_id = 0;

    int row_extent = 0;
    int row_baseline = 0;
    int row_tallest = 0;

    // DEBUG: Color atlas background to spot incorrect shaders easier.
    std::vector<uint8_t> atlas_background;

    bool roomInRow(int width, int height);
    bool advanceRow();
};

}
