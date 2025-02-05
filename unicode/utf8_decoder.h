#pragma once

#include <cstdint>

namespace unicode {

// https://github.com/gershnik/sys_string/blob/f6d5127da833ecf99b499156a9bc0f2093f2f745/lib/inc/sys_string/impl/unicode/utf_encoding.h#L232

// https://gershnik.github.io/2021/03/24/reverse-utf8-decoding.html
class ReverseUTF8Decoder {
public:
    constexpr void put(uint8_t byte) {
        uint32_t type = kStateTable[byte];
        state = kStateTable[256 + state + type];

        if (state <= kRejectState) {
            collect |= (((0xffu >> type) & (byte)) << shift);
            value_ = char32_t(collect);
            shift = 0;
            collect = 0;
        } else {
            collect |= ((byte & 0x3fu) << shift);
            shift += 6;
        }
    }

    constexpr bool done() const {
        return state == kAcceptState;
    }

    constexpr bool error() const {
        return state == kRejectState;
    }

    constexpr char32_t value() const {
        return value_;
    }

private:
    static constexpr uint8_t kAcceptState = 0;
    static constexpr uint8_t kRejectState = 12;
    char32_t value_ = 0;
    uint32_t collect = 0;
    uint8_t shift = 0;
    uint8_t state = kAcceptState;

    // TODO: Format state table correctly.
    static constexpr const uint8_t kStateTable[] = {
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
        1,  1,  1,  1,  1,  1,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  7,
        7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,
        7,  7,  7,  7,  7,  7,  7,  7,  8,  8,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
        2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  10, 3,  3,  3,  3,  3,
        3,  3,  3,  3,  3,  3,  3,  4,  3,  3,  11, 6,  6,  6,  5,  8,  8,  8,  8,  8,  8,  8,  8,
        8,  8,  8,  0,  24, 12, 12, 12, 12, 12, 24, 12, 24, 12, 12, 0,  24, 12, 12, 12, 12, 12, 24,
        12, 24, 12, 12, 12, 36, 0,  12, 12, 12, 12, 48, 12, 36, 12, 12, 12, 60, 12, 0,  0,  12, 12,
        72, 12, 72, 12, 12, 12, 60, 12, 0,  12, 12, 12, 72, 12, 72, 0,  12, 12, 12, 12, 12, 12, 0,
        0,  12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 0};
};

}  // namespace unicode
