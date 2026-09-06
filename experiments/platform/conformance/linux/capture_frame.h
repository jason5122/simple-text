#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace linux_capture {

struct Crop {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

struct Frame {
    int width = 0;
    int height = 0;
    std::vector<std::uint8_t> rgba;

    bool operator==(const Frame&) const = default;
};

class Source {
public:
    virtual ~Source() = default;

    // Ratio between compositor device pixels and logical desktop coordinates.
    virtual double device_scale() const { return 1.0; }

    // Returns the next compositor frame, cropped in device pixels. X11 sources sample their
    // redirected pixmap; Wayland sources wait for the next PipeWire buffer.
    virtual bool next_frame(Frame* frame, Crop crop, std::chrono::milliseconds timeout) = 0;
};

std::unique_ptr<Source> make_mutter_source(std::string* error);
std::unique_ptr<Source> make_x11_source(int pid, const char* owner, std::string* error);

bool write_png(const Frame& frame, const char* path);

}  // namespace linux_capture
