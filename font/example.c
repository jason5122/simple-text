#include "third_party/libgrapheme/grapheme.h"
#include <stdint.h>
#include <stdio.h>

void libgraphemeExample() {
    /* UTF-8 encoded input */
    const char* s = "T\xC3\xABst \xF0\x9F\x91\xA8\xE2\x80\x8D\xF0"
                    "\x9F\x91\xA9\xE2\x80\x8D\xF0\x9F\x91\xA6 \xF0"
                    "\x9F\x87\xBA\xF0\x9F\x87\xB8 \xE0\xA4\xA8\xE0"
                    "\xA5\x80 \xE0\xAE\xA8\xE0\xAE\xBF!";
    size_t ret;

    printf("Input: \"%s\"\n", s);

    /* print each grapheme cluster with byte-length */
    printf("grapheme clusters in NUL-delimited input:\n");
    for (size_t offset = 0; s[offset] != '\0'; offset += ret) {
        ret = grapheme_next_character_break_utf8(s + offset, SIZE_MAX);
        printf("%2zu bytes | %.*s\n", ret, (int)ret, s + offset);
    }
    printf("\n");

    /* do the same, but this time string is length-delimited */
    size_t len = 17;
    printf("grapheme clusters in input delimited to %zu bytes:\n", len);
    for (size_t offset = 0; offset < len; offset += ret) {
        ret = grapheme_next_character_break_utf8(s + offset, len - offset);
        printf("%2zu bytes | %.*s\n", ret, (int)ret, s + offset);
    }
}
