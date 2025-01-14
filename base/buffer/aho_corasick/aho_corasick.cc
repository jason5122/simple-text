#include "aho_corasick.h"

#include "ac_fast.h"
#include "ac_slow.h"

#include <fmt/base.h>

namespace base {

class BufAlloc : public Buf_Allocator {
public:
    virtual AC_Buffer* alloc(int sz) {
        return (AC_Buffer*)(new unsigned char[sz]);
    }

    // Do not de-allocate the buffer when the BufAlloc die.
    virtual void free() {}

    static void myfree(AC_Buffer* buf) {
        const char* b = (const char*)buf;
        delete[] b;
    }
};

AhoCorasick::AhoCorasick(const std::vector<std::string>& patterns) {
    if (patterns.size() >= 65535) {
        // TODO: Currently we use 16-bit to encode pattern-index (see the comment to
        // AC_State::is_term), therefore we are not able to handle pattern set with more than 65535
        // entries.
        fmt::println("Error: Pattern limit of 65535 exceeded in AhoCorasick constructor.");
        std::abort();
    }

    ACS_Constructor acc;
    acc.Construct(patterns);

    BufAlloc ba;
    AC_Converter cvt(acc, ba);
    AC_Buffer* buf = cvt.Convert();
    this->buf = (void*)buf;
}

AhoCorasick::~AhoCorasick() {
    AC_Buffer* buf = (AC_Buffer*)this->buf;
    BufAlloc::myfree(buf);
}

AhoCorasick::MatchResult AhoCorasick::match(std::string_view str, unsigned int len) const {
    AC_Buffer* buf = (AC_Buffer*)this->buf;
    return Match(buf, str, len);
}

AhoCorasick::MatchResult AhoCorasick::match(const PieceTree& tree) const {
    AC_Buffer* buf = (AC_Buffer*)this->buf;
    return Match(buf, tree);
}

}  // namespace base
