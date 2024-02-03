#pragma once

inline uint32_t UTF8StringToScalar(const char* str) {
    uint32_t unicode_scalar = 0;
    for (size_t i = 0; i < 4 && str[i] != '\0'; i++) {
        uint8_t byte = str[i];
        unicode_scalar |= byte << 8 * i;
    }
    return unicode_scalar;
}
