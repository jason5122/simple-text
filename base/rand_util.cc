#include "base/containers/span_util.h"
#include "base/rand_util.h"

namespace base {

uint64_t rand_uint64() {
    uint64_t number;
    rand_bytes(as_writable_u8_span(number));
    return number;
}

int rand_int(int min, int max) {
    DCHECK_LE(min, max);

    uint64_t range = static_cast<uint64_t>(max) - static_cast<uint64_t>(min) + 1;
    // |range| is at most UINT_MAX + 1, so the result of RandGenerator(range)
    // is at most UINT_MAX.  Hence it's safe to cast it from uint64_t to int64_t.
    int result = static_cast<int>(min + static_cast<int64_t>(rand_generator(range)));
    DCHECK_GE(result, min);
    DCHECK_LE(result, max);
    return result;
}

uint64_t rand_generator(uint64_t range) {
    DCHECK_GT(range, 0u);
    // We must discard random results above this number, as they would
    // make the random generator non-uniform (consider e.g. if
    // MAX_UINT64 was 7 and |range| was 5, then a result of 1 would be twice
    // as likely as a result of 3 or 4).
    uint64_t max_acceptable_value = (std::numeric_limits<uint64_t>::max() / range) * range - 1;

    uint64_t value;
    do {
        value = rand_uint64();
    } while (value > max_acceptable_value);

    return value % range;
}

std::string rand_bytes_as_string(size_t length) {
    std::string result(length, '\0');
    rand_bytes(as_writable_u8_span(result));
    return result;
}

std::vector<uint8_t> rand_bytes_as_vector(size_t length) {
    std::vector<uint8_t> result(length);
    rand_bytes(result);
    return result;
}

}  // namespace base
