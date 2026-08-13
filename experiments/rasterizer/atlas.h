#pragma once

#include <OpenGL/gl3.h>
#include <cstdint>
#include <vector>

// Minimal shelf-packed glyph atlas: a single RGBA texture that glyph bitmaps are uploaded into
// left-to-right, wrapping to a new row when the current one fills. No eviction -- entries live for
// the atlas's lifetime, which is all the demo's fixed corpus needs.
class Atlas {
public:
    static constexpr int kSize = 1024;

    struct UV {
        float x, y, w, h;  // normalized to [0, 1]
    };

    Atlas();  // issues GL calls; requires a current context
    ~Atlas();
    Atlas(const Atlas&) = delete;
    Atlas& operator=(const Atlas&) = delete;

    GLuint tex() const { return tex_id_; }

    // Uploads one premultiplied-BGRA (host byte order) bitmap and returns its atlas rect. Returns
    // false if the glyph can't fit.
    bool insert(int width, int height, const std::vector<uint8_t>& pixels, UV& out_uv);

private:
    bool fits_in_row(int width, int height) const;
    bool advance_row();

    GLuint tex_id_ = 0;
    int row_x_ = 0;       // pen x within the current row
    int row_y_ = 0;       // top edge of the current row
    int row_height_ = 0;  // tallest glyph placed in the current row
};
