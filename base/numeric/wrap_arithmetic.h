#pragma once

#include <cassert>
#include <cstddef>
#include <limits>

namespace {

constexpr int min_int = std::numeric_limits<int>::min();
constexpr int max_int = std::numeric_limits<int>::max();
constexpr size_t min_size_t = std::numeric_limits<size_t>::min();
constexpr size_t max_size_t = std::numeric_limits<size_t>::max();

}

namespace base {

constexpr size_t inc_wrap(size_t i, size_t size) noexcept {
    assert(i < size);

    if (i + 1 == size) {
        return 0;
    } else {
        return i + 1;
    }
}

constexpr size_t dec_wrap(size_t i, size_t size) noexcept {
    assert(i < size);

    if (i == 0) {
        return size - 1;
    } else {
        return i - 1;
    }
}

static_assert(inc_wrap(0UL, 3UL) == 1UL);
static_assert(inc_wrap(1UL, 3UL) == 2UL);
static_assert(inc_wrap(2UL, 3UL) == 0UL);
static_assert(inc_wrap(0UL, 1UL) == 0UL);
static_assert(inc_wrap(max_size_t - 1, max_size_t) == 0UL);

static_assert(dec_wrap(0UL, 3UL) == 2UL);
static_assert(dec_wrap(1UL, 3UL) == 0UL);
static_assert(dec_wrap(2UL, 3UL) == 1UL);
static_assert(dec_wrap(0UL, 1UL) == 0UL);
static_assert(dec_wrap(0, max_size_t) == max_size_t - 1);

}
