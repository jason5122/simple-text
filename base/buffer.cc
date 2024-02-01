#include "buffer.h"
#include <cstdint>
#include <sstream>

Buffer::Buffer(std::string s) {
    std::stringstream ss(s);
    std::string line;
    while (std::getline(ss, line, '\n')) {
        data.push_back(line);
    }
}

Buffer::Buffer(std::ifstream& istrm) {
    std::string line;
    while (std::getline(istrm, line, '\n')) {
        data.push_back(line);
    }
}

size_t Buffer::lineCount() {
    return data.size();
}

std::string Buffer::line(size_t line_index) {
    return data[line_index];
}

size_t Buffer::byteOfLine(size_t line_index) {
    size_t byte_offset = 0;
    for (uint16_t row = 0; row < line_index; row++) {
        byte_offset += data[row].size();
        byte_offset++;  // Include newline.
    }
    return byte_offset;
}

// FIXME: This is SLOW! This is a naïve implementation for testing.
const char* Buffer::toCString() {
    // std::stringstream ss;
    // for (const std::string& line : data) {
    //     ss << line;
    //     ss << '\n';
    // }
    // const std::string& tmp = ss.str();
    // const char* cstr = tmp.c_str();
    // return cstr;
    return R"((define WINDOW-WIDTH 150)
(define WINDOW-HEIGHT 180)

; A world is a number.
; Its display is a blue disk of that size.
; show-world : world -> image
(define (show-world diameter)
  (circle diameter "solid" "blue"))
"Examples of show-world:"
(show-world 1) "should be a blue dot"
(show-world 20) "should be" (circle 20 "solid" "blue")

"Now let's start the animation!"

(big-bang WINDOW-WIDTH WINDOW-HEIGHT 1 10)
(on-update-event show-world)
)";
}
