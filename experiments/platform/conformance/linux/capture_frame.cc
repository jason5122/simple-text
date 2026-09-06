#include "experiments/platform/conformance/linux/capture_frame.h"

#include <cstdio>
#include <memory>

#include <spng.h>

namespace linux_capture {

bool write_png(const Frame& frame, const char* path) {
    if (frame.width <= 0 || frame.height <= 0 ||
        frame.rgba.size() !=
            static_cast<size_t>(frame.width) * static_cast<size_t>(frame.height) * 4) {
        return false;
    }

    std::unique_ptr<FILE, decltype(&std::fclose)> file(std::fopen(path, "wb"), &std::fclose);
    std::unique_ptr<spng_ctx, decltype(&spng_ctx_free)> context(
        spng_ctx_new(SPNG_CTX_ENCODER), &spng_ctx_free);
    if (!file || !context) {
        return false;
    }
    int result = spng_set_png_file(context.get(), file.get());
    if (result != 0) {
        std::fprintf(stderr, "spng_set_png_file: %s\n", spng_strerror(result));
        return false;
    }

    spng_ihdr header{};
    header.width = static_cast<uint32_t>(frame.width);
    header.height = static_cast<uint32_t>(frame.height);
    header.bit_depth = 8;
    header.color_type = SPNG_COLOR_TYPE_TRUECOLOR_ALPHA;
    result = spng_set_ihdr(context.get(), &header);
    if (result != 0) {
        std::fprintf(stderr, "spng_set_ihdr: %s\n", spng_strerror(result));
        return false;
    }
    result = spng_encode_image(context.get(), frame.rgba.data(), frame.rgba.size(), SPNG_FMT_PNG,
                               SPNG_ENCODE_FINALIZE);
    if (result != 0) {
        std::fprintf(stderr, "spng_encode_image: %s\n", spng_strerror(result));
        return false;
    }
    return true;
}

}  // namespace linux_capture
