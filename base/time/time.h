#pragma once

#include <compare>
#include <cstdint>
#include <limits>

namespace base {

namespace Time {

constexpr int64_t kMillisecondsPerSecond = 1000;
constexpr int64_t kMicrosecondsPerMillisecond = 1000;
constexpr int64_t kMicrosecondsPerSecond = kMicrosecondsPerMillisecond * kMillisecondsPerSecond;
constexpr int64_t kNanosecondsPerMicrosecond = 1000;
constexpr int64_t kNanosecondsPerSecond = kNanosecondsPerMicrosecond * kMicrosecondsPerSecond;

}  // namespace Time

class TimeDelta {
public:
    constexpr TimeDelta() = default;

    static constexpr TimeDelta seconds(int64_t us) {
        return TimeDelta(us) * Time::kMicrosecondsPerSecond;
    }
    static constexpr TimeDelta milliseconds(int64_t us) {
        return TimeDelta(us) * Time::kMicrosecondsPerMillisecond;
    }
    static constexpr TimeDelta microseconds(int64_t us) { return TimeDelta(us); }
    static constexpr TimeDelta nanoseconds(int64_t us) {
        return TimeDelta(us) * Time::kNanosecondsPerMicrosecond;
    }

    constexpr double in_seconds_f() const {
        return static_cast<double>(delta_) / Time::kMicrosecondsPerSecond;
    }
    constexpr int64_t in_microseconds() const { return delta_; }

    constexpr bool is_zero() const { return delta_ == 0; }
    constexpr bool is_positive() const { return delta_ > 0; }
    constexpr bool is_negative() const { return delta_ < 0; }

    // Computations with other deltas.
    constexpr TimeDelta operator+(TimeDelta t) const { return TimeDelta(delta_ + t.delta_); }
    constexpr TimeDelta operator-(TimeDelta t) const { return TimeDelta(delta_ - t.delta_); }
    constexpr TimeDelta& operator+=(TimeDelta t) { return *this = (*this + t); }
    constexpr TimeDelta& operator-=(TimeDelta t) { return *this = (*this - t); }
    constexpr TimeDelta operator-() const { return TimeDelta(-delta_); }

    // Computations with numeric types.
    constexpr TimeDelta operator*(int64_t a) const { return TimeDelta(delta_ * a); }
    constexpr TimeDelta operator/(int64_t a) const { return TimeDelta(delta_ / a); }
    constexpr TimeDelta& operator*=(int64_t a) { return *this = (*this * a); }
    constexpr TimeDelta& operator/=(int64_t a) { return *this = (*this / a); }

    // Comparison operators.
    friend constexpr bool operator==(TimeDelta, TimeDelta) = default;
    friend constexpr auto operator<=>(TimeDelta, TimeDelta) = default;

private:
    // Constructs a delta given the duration in microseconds. This is private to avoid confusion by
    // callers with an integer constructor. Use base::Seconds, base::Milliseconds, etc. instead.
    constexpr explicit TimeDelta(int64_t delta_us) : delta_(delta_us) {}

    // Delta in microseconds.
    int64_t delta_ = 0;
};

class TimeTicks {
public:
    constexpr TimeTicks() : us_(0) {}

    static TimeTicks now();

    constexpr bool is_null() const { return us_ == 0; }
    constexpr bool is_max() const { return us_ == std::numeric_limits<int64_t>::max(); }
    constexpr bool is_min() const { return us_ == std::numeric_limits<int64_t>::min(); }

    static constexpr TimeTicks max() { return TimeTicks(std::numeric_limits<int64_t>::max()); }
    static constexpr TimeTicks min() { return TimeTicks(std::numeric_limits<int64_t>::min()); }

    constexpr TimeDelta operator-(const TimeTicks& other) const {
        return TimeDelta::microseconds(us_ - other.us_);
    }

    constexpr TimeTicks operator+(TimeDelta delta) const {
        return TimeTicks((TimeDelta::microseconds(us_) + delta).in_microseconds());
    }
    constexpr TimeTicks operator-(TimeDelta delta) const {
        return TimeTicks((TimeDelta::microseconds(us_) - delta).in_microseconds());
    }
    constexpr TimeTicks& operator+=(TimeDelta delta) { return *this = (*this + delta); }
    constexpr TimeTicks& operator-=(TimeDelta delta) { return *this = (*this - delta); }

    friend constexpr bool operator==(const TimeTicks&, const TimeTicks&) = default;
    friend constexpr auto operator<=>(const TimeTicks&, const TimeTicks&) = default;

private:
    constexpr explicit TimeTicks(int64_t us) : us_(us) {}

    int64_t us_;
};

}  // namespace base
