#include "ac.h"

#include "ac_fast.h"
#include "ac_slow.h"

#include <cassert>

int ac_match(ac_t* ac, const char* str, unsigned int len) {
    AC_Buffer* buf = (AC_Buffer*)(void*)ac;
    ac_result_t r = Match(buf, str, len);
    return r.match_begin;
}

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

ac_t* ac_create(const char** strv, unsigned int* strlenv, unsigned int v_len) {
    if (v_len >= 65535) {
        // TODO: Currently we use 16-bit to encode pattern-index (see the
        //  comment to AC_State::is_term), therefore we are not able to
        //  handle pattern set with more than 65535 entries.
        return 0;
    }

    ACS_Constructor* acc;
    ACS_Constructor tmp;
    acc = &tmp;
    acc->Construct(strv, strlenv, v_len);

    BufAlloc ba;
    AC_Converter cvt(*acc, ba);
    AC_Buffer* buf = cvt.Convert();
    return (ac_t*)(void*)buf;
}

void ac_free(void* ac) {
    AC_Buffer* buf = (AC_Buffer*)ac;
    BufAlloc::myfree(buf);
}
