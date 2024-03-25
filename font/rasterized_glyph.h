#pragma once

#include <cstdint>
#include <vector>

struct RasterizedGlyph {
    bool colored;
    int32_t left;
    int32_t top;
    int32_t width;
    int32_t height;
    float advance;
    std::vector<uint8_t> buffer;
    // TODO: Either remove these debug fields, or evaluate if we should keep them.
    unsigned short index;
};
