#include "base/check.h"
#include "base/numeric/safe_conversions.h"
#include "base/time/time.h"
#include <mach/mach_time.h>

namespace base {

namespace {

// Returns a pointer to the initialized Mach timebase info struct.
mach_timebase_info_data_t* mach_timebase_info() {
    static mach_timebase_info_data_t timebase_info = [] {
        mach_timebase_info_data_t info;
        kern_return_t kr = mach_timebase_info(&info);
        DCHECK(kr == KERN_SUCCESS);
        DCHECK(info.numer);
        DCHECK(info.denom);
        return info;
    }();
    return &timebase_info;
}

int64_t mach_time_to_microseconds(uint64_t mach_time) {
    // timebase_info gives us the conversion factor between absolute time tick
    // units and nanoseconds.
    mach_timebase_info_data_t* timebase_info = mach_timebase_info();

    // Take the fast path when the conversion is 1:1. The result will for sure fit
    // into an int_64 because we're going from nanoseconds to microseconds.
    if (timebase_info->numer == timebase_info->denom) {
        return static_cast<int64_t>(mach_time / Time::kNanosecondsPerMicrosecond);
    }

    uint64_t microseconds = 0;
    const uint64_t divisor = timebase_info->denom * Time::kNanosecondsPerMicrosecond;

    // Microseconds is mach_time * timebase.numer /
    // (timebase.denom * kNanosecondsPerMicrosecond). Divide first to reduce
    // the chance of overflow. Also stash the remainder right now, a likely
    // byproduct of the division.
    microseconds = mach_time / divisor;
    const uint64_t mach_time_remainder = mach_time % divisor;

    // Now multiply, keeping an eye out for overflow.
    CHECK(!__builtin_umulll_overflow(microseconds, timebase_info->numer, &microseconds));

    // By dividing first we lose precision. Regain it by adding back the
    // microseconds from the remainder, with an eye out for overflow.
    uint64_t least_significant_microseconds =
        (mach_time_remainder * timebase_info->numer) / divisor;
    CHECK(!__builtin_uaddll_overflow(microseconds, least_significant_microseconds, &microseconds));

    // Don't bother with the rollover handling that the Windows version does.
    // The returned time in microseconds is enough for 292,277 years (starting
    // from 2^63 because the returned int64_t is signed,
    // 9223372036854775807 / (1e6 * 60 * 60 * 24 * 365.2425) = 292,277).
    return checked_cast<int64_t>(microseconds);
}

// Returns monotonically growing number of ticks in microseconds since some
// unspecified starting point.
int64_t compute_current_ticks() {
    // mach_absolute_time is it when it comes to ticks on the Mac.  Other calls
    // with less precision (such as TickCount) just call through to
    // mach_absolute_time.
    return mach_time_to_microseconds(mach_absolute_time());
}

}  // namespace

TimeTicks TimeTicks::now() {
    return TimeTicks() + TimeDelta::microseconds(compute_current_ticks());
}

}  // namespace base
