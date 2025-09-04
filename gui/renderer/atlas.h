#pragma once

#include "gl/gl.h"
#include "gui/renderer/types.h"
#include <vector>

namespace gui {

class Atlas {
public:
    // 1024 is a conservative size.
    // https://feedback.wildfiregames.com/report/opengl/feature/GL_MAX_TEXTURE_SIZE
    // static constexpr int kAtlasSize = 1024;
    // TODO: Setting this to a smaller size reduces initial memory usage. Figure out how to balance
    // that with performance (a larger texture size likely means less texture swaps).
    static constexpr int kAtlasSize = 2048;
    // static constexpr int kAtlasSize = 4096;
    // static constexpr int kAtlasSize = 16384;

    Atlas();
    ~Atlas();
    Atlas(const Atlas&) = delete;
    Atlas& operator=(const Atlas&) = delete;
    Atlas(Atlas&&) noexcept;
    Atlas& operator=(Atlas&&) noexcept;

    gl::GLuint tex() const;

    enum class Format {
        kBGRA,
        kRGBA,
        kRGB,
    };
    bool insert_texture(
        int width, int height, Format format, const std::vector<uint8_t>& data, Vec4& out_uv);

private:
    gl::GLuint tex_id = 0;

    int row_extent = 0;
    int row_baseline = 0;
    int row_tallest = 0;

    // DEBUG: Color atlas background to spot incorrect shaders easier.
    std::vector<uint8_t> atlas_background;

    bool room_in_row(int width, int height);
    bool advance_row();
};

static_assert(std::is_nothrow_destructible_v<Atlas>);
static_assert(!std::is_copy_constructible_v<Atlas>);
static_assert(!std::is_copy_assignable_v<Atlas>);
static_assert(std::is_nothrow_move_constructible_v<Atlas>);
static_assert(std::is_nothrow_move_assignable_v<Atlas>);

}  // namespace gui
