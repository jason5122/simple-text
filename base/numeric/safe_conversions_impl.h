#pragma once

#include <limits>
#include <type_traits>

#if defined(__GNUC__) || defined(__clang__)
#define BASE_NUMERICS_LIKELY(x) __builtin_expect(!!(x), 1)
#define BASE_NUMERICS_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define BASE_NUMERICS_LIKELY(x) (x)
#define BASE_NUMERICS_UNLIKELY(x) (x)
#endif

namespace base::internal {

// The std library doesn't provide a binary max_exponent for integers, however we can compute an
// analog using std::numeric_limits<>::digits.
template <typename NumericType>
struct MaxExponent {
    static const int value = std::is_floating_point_v<NumericType>
                                 ? std::numeric_limits<NumericType>::max_exponent
                                 : std::numeric_limits<NumericType>::digits + 1;
};

// Forces a crash, like a CHECK(false). Used for numeric boundary errors.
// Also used in a constexpr template to trigger a compilation failure on
// an error condition.
struct CheckOnFailure {
    template <typename T>
    static T HandleFailure() {
#if defined(_MSC_VER)
        __debugbreak();
#elif defined(__GNUC__) || defined(__clang__)
        __builtin_trap();
#else
        ((void)(*(volatile char*)0 = 0));
#endif
        return T();
    }
};

enum IntegerRepresentation { INTEGER_REPRESENTATION_UNSIGNED, INTEGER_REPRESENTATION_SIGNED };

// A range for a given numeric Src type is contained for a given numeric Dst type if both
// numeric_limits<Src>::max() <= numeric_limits<Dst>::max() and numeric_limits<Src>::lowest() >=
// numeric_limits<Dst>::lowest() are true. We implement this as template specializations rather
// than simple static comparisons to ensure type correctness in our comparisons.
enum NumericRangeRepresentation { NUMERIC_RANGE_NOT_CONTAINED, NUMERIC_RANGE_CONTAINED };

// Helper templates to statically determine if our destination type can contain maximum and minimum
// values represented by the source type.

template <typename Dst,
          typename Src,
          IntegerRepresentation DstSign = std::is_signed_v<Dst> ? INTEGER_REPRESENTATION_SIGNED
                                                                : INTEGER_REPRESENTATION_UNSIGNED,
          IntegerRepresentation SrcSign = std::is_signed_v<Src> ? INTEGER_REPRESENTATION_SIGNED
                                                                : INTEGER_REPRESENTATION_UNSIGNED>
struct StaticDstRangeRelationToSrcRange;

// Same sign: Dst is guaranteed to contain Src only if its range is equal or larger.
template <typename Dst, typename Src, IntegerRepresentation Sign>
struct StaticDstRangeRelationToSrcRange<Dst, Src, Sign, Sign> {
    static const NumericRangeRepresentation value =
        MaxExponent<Dst>::value >= MaxExponent<Src>::value ? NUMERIC_RANGE_CONTAINED
                                                           : NUMERIC_RANGE_NOT_CONTAINED;
};

// Unsigned to signed: Dst is guaranteed to contain source only if its range is larger.
template <typename Dst, typename Src>
struct StaticDstRangeRelationToSrcRange<Dst,
                                        Src,
                                        INTEGER_REPRESENTATION_SIGNED,
                                        INTEGER_REPRESENTATION_UNSIGNED> {
    static const NumericRangeRepresentation value =
        MaxExponent<Dst>::value > MaxExponent<Src>::value ? NUMERIC_RANGE_CONTAINED
                                                          : NUMERIC_RANGE_NOT_CONTAINED;
};

// Signed to unsigned: Dst cannot be statically determined to contain Src.
template <typename Dst, typename Src>
struct StaticDstRangeRelationToSrcRange<Dst,
                                        Src,
                                        INTEGER_REPRESENTATION_UNSIGNED,
                                        INTEGER_REPRESENTATION_SIGNED> {
    static const NumericRangeRepresentation value = NUMERIC_RANGE_NOT_CONTAINED;
};

// This class wraps the range constraints as separate booleans so the compiler
// can identify constants and eliminate unused code paths.
class RangeCheck {
public:
    constexpr RangeCheck(bool is_in_lower_bound, bool is_in_upper_bound)
        : is_underflow_(!is_in_lower_bound), is_overflow_(!is_in_upper_bound) {}
    constexpr RangeCheck() : is_underflow_(false), is_overflow_(false) {}
    constexpr bool IsValid() const {
        return !is_overflow_ && !is_underflow_;
    }
    constexpr bool IsInvalid() const {
        return is_overflow_ && is_underflow_;
    }
    constexpr bool IsOverflow() const {
        return is_overflow_ && !is_underflow_;
    }
    constexpr bool IsUnderflow() const {
        return !is_overflow_ && is_underflow_;
    }
    constexpr bool IsOverflowFlagSet() const {
        return is_overflow_;
    }
    constexpr bool IsUnderflowFlagSet() const {
        return is_underflow_;
    }
    constexpr bool operator==(const RangeCheck rhs) const {
        return is_underflow_ == rhs.is_underflow_ && is_overflow_ == rhs.is_overflow_;
    }
    constexpr bool operator!=(const RangeCheck rhs) const {
        return !(*this == rhs);
    }

private:
    // Do not change the order of these member variables. The integral conversion
    // optimization depends on this exact order.
    const bool is_underflow_;
    const bool is_overflow_;
};

// Determines if a numeric value is negative without throwing compiler
// warnings on: unsigned(value) < 0.
template <typename T, std::enable_if_t<std::is_signed_v<T>>* = nullptr>
constexpr bool IsValueNegative(T value) {
    static_assert(std::is_arithmetic_v<T>, "Argument must be numeric.");
    return value < 0;
}

template <typename T, std::enable_if_t<!std::is_signed_v<T>>* = nullptr>
constexpr bool IsValueNegative(T) {
    static_assert(std::is_arithmetic_v<T>, "Argument must be numeric.");
    return false;
}

// This performs a fast negation, returning a signed value. It works on unsigned
// arguments, but probably doesn't do what you want for any unsigned value
// larger than max / 2 + 1 (i.e. signed min cast to unsigned).
template <typename T>
constexpr typename std::make_signed<T>::type ConditionalNegate(T x, bool is_negative) {
    static_assert(std::is_integral_v<T>, "Type must be integral");
    using SignedT = typename std::make_signed<T>::type;
    using UnsignedT = typename std::make_unsigned<T>::type;
    return static_cast<SignedT>(
        (static_cast<UnsignedT>(x) ^ static_cast<UnsignedT>(-SignedT(is_negative))) + is_negative);
}

// This performs a safe, absolute value via unsigned overflow.
template <typename T>
constexpr typename std::make_unsigned<T>::type SafeUnsignedAbs(T value) {
    static_assert(std::is_integral_v<T>, "Type must be integral");
    using UnsignedT = typename std::make_unsigned<T>::type;
    return IsValueNegative(value) ? static_cast<UnsignedT>(0u - static_cast<UnsignedT>(value))
                                  : static_cast<UnsignedT>(value);
}

// The following helper template addresses a corner case in range checks for
// conversion from a floating-point type to an integral type of smaller range
// but larger precision (e.g. float -> unsigned). The problem is as follows:
//   1. Integral maximum is always one less than a power of two, so it must be
//      truncated to fit the mantissa of the floating point. The direction of
//      rounding is implementation defined, but by default it's always IEEE
//      floats, which round to nearest and thus result in a value of larger
//      magnitude than the integral value.
//      Example: float f = UINT_MAX; // f is 4294967296f but UINT_MAX
//                                   // is 4294967295u.
//   2. If the floating point value is equal to the promoted integral maximum
//      value, a range check will erroneously pass.
//      Example: (4294967296f <= 4294967295u) // This is true due to a precision
//                                            // loss in rounding up to float.
//   3. When the floating point value is then converted to an integral, the
//      resulting value is out of range for the target integral type and
//      thus is implementation defined.
//      Example: unsigned u = (float)INT_MAX; // u will typically overflow to 0.
// To fix this bug we manually truncate the maximum value when the destination
// type is an integral of larger precision than the source floating-point type,
// such that the resulting maximum is represented exactly as a floating point.
template <typename Dst, typename Src, template <typename> class Bounds>
struct NarrowingRange {
    using SrcLimits = std::numeric_limits<Src>;
    using DstLimits = typename std::numeric_limits<Dst>;

    // Computes the mask required to make an accurate comparison between types.
    static const int kShift = (MaxExponent<Src>::value > MaxExponent<Dst>::value &&
                               SrcLimits::digits < DstLimits::digits)
                                  ? (DstLimits::digits - SrcLimits::digits)
                                  : 0;
    template <typename T, std::enable_if_t<std::is_integral_v<T>>* = nullptr>

    // Masks out the integer bits that are beyond the precision of the
    // intermediate type used for comparison.
    static constexpr T Adjust(T value) {
        static_assert(std::is_same_v<T, Dst>, "");
        static_assert(kShift < DstLimits::digits, "");
        using UnsignedDst = typename std::make_unsigned_t<T>;
        return static_cast<T>(ConditionalNegate(SafeUnsignedAbs(value) &
                                                    ~((UnsignedDst{1} << kShift) - UnsignedDst{1}),
                                                IsValueNegative(value)));
    }

    template <typename T, std::enable_if_t<std::is_floating_point_v<T>>* = nullptr>
    static constexpr T Adjust(T value) {
        static_assert(std::is_same_v<T, Dst>, "");
        static_assert(kShift == 0, "");
        return value;
    }

    static constexpr Dst max() {
        return Adjust(Bounds<Dst>::max());
    }
    static constexpr Dst lowest() {
        return Adjust(Bounds<Dst>::lowest());
    }
};

// Simple wrapper for statically checking if a type's range is contained.
template <typename Dst, typename Src>
struct IsTypeInRangeForNumericType {
    static const bool value =
        StaticDstRangeRelationToSrcRange<Dst, Src>::value == NUMERIC_RANGE_CONTAINED;
};

template <typename Dst,
          typename Src,
          template <typename> class Bounds,
          IntegerRepresentation DstSign = std::is_signed_v<Dst> ? INTEGER_REPRESENTATION_SIGNED
                                                                : INTEGER_REPRESENTATION_UNSIGNED,
          IntegerRepresentation SrcSign = std::is_signed_v<Src> ? INTEGER_REPRESENTATION_SIGNED
                                                                : INTEGER_REPRESENTATION_UNSIGNED,
          NumericRangeRepresentation DstRange = StaticDstRangeRelationToSrcRange<Dst, Src>::value>
struct DstRangeRelationToSrcRangeImpl;

// The following templates are for ranges that must be verified at runtime. We
// split it into checks based on signedness to avoid confusing casts and
// compiler warnings on signed an unsigned comparisons.

// Same sign narrowing: The range is contained for normal limits.
template <typename Dst,
          typename Src,
          template <typename> class Bounds,
          IntegerRepresentation DstSign,
          IntegerRepresentation SrcSign>
struct DstRangeRelationToSrcRangeImpl<Dst,
                                      Src,
                                      Bounds,
                                      DstSign,
                                      SrcSign,
                                      NUMERIC_RANGE_CONTAINED> {
    static constexpr RangeCheck Check(Src value) {
        using SrcLimits = std::numeric_limits<Src>;
        using DstLimits = NarrowingRange<Dst, Src, Bounds>;
        return RangeCheck(static_cast<Dst>(SrcLimits::lowest()) >= DstLimits::lowest() ||
                              static_cast<Dst>(value) >= DstLimits::lowest(),
                          static_cast<Dst>(SrcLimits::max()) <= DstLimits::max() ||
                              static_cast<Dst>(value) <= DstLimits::max());
    }
};

// Signed to signed narrowing: Both the upper and lower boundaries may be
// exceeded for standard limits.
template <typename Dst, typename Src, template <typename> class Bounds>
struct DstRangeRelationToSrcRangeImpl<Dst,
                                      Src,
                                      Bounds,
                                      INTEGER_REPRESENTATION_SIGNED,
                                      INTEGER_REPRESENTATION_SIGNED,
                                      NUMERIC_RANGE_NOT_CONTAINED> {
    static constexpr RangeCheck Check(Src value) {
        using DstLimits = NarrowingRange<Dst, Src, Bounds>;
        return RangeCheck(value >= DstLimits::lowest(), value <= DstLimits::max());
    }
};

// Unsigned to unsigned narrowing: Only the upper bound can be exceeded for
// standard limits.
template <typename Dst, typename Src, template <typename> class Bounds>
struct DstRangeRelationToSrcRangeImpl<Dst,
                                      Src,
                                      Bounds,
                                      INTEGER_REPRESENTATION_UNSIGNED,
                                      INTEGER_REPRESENTATION_UNSIGNED,
                                      NUMERIC_RANGE_NOT_CONTAINED> {
    static constexpr RangeCheck Check(Src value) {
        using DstLimits = NarrowingRange<Dst, Src, Bounds>;
        return RangeCheck(DstLimits::lowest() == Dst(0) || value >= DstLimits::lowest(),
                          value <= DstLimits::max());
    }
};

// Unsigned to signed: Only the upper bound can be exceeded for standard limits.
template <typename Dst, typename Src, template <typename> class Bounds>
struct DstRangeRelationToSrcRangeImpl<Dst,
                                      Src,
                                      Bounds,
                                      INTEGER_REPRESENTATION_SIGNED,
                                      INTEGER_REPRESENTATION_UNSIGNED,
                                      NUMERIC_RANGE_NOT_CONTAINED> {
    static constexpr RangeCheck Check(Src value) {
        using DstLimits = NarrowingRange<Dst, Src, Bounds>;
        using Promotion = decltype(Src() + Dst());
        return RangeCheck(
            DstLimits::lowest() <= Dst(0) ||
                static_cast<Promotion>(value) >= static_cast<Promotion>(DstLimits::lowest()),
            static_cast<Promotion>(value) <= static_cast<Promotion>(DstLimits::max()));
    }
};

// Signed to unsigned: The upper boundary may be exceeded for a narrower Dst,
// and any negative value exceeds the lower boundary for standard limits.
template <typename Dst, typename Src, template <typename> class Bounds>
struct DstRangeRelationToSrcRangeImpl<Dst,
                                      Src,
                                      Bounds,
                                      INTEGER_REPRESENTATION_UNSIGNED,
                                      INTEGER_REPRESENTATION_SIGNED,
                                      NUMERIC_RANGE_NOT_CONTAINED> {
    static constexpr RangeCheck Check(Src value) {
        using SrcLimits = std::numeric_limits<Src>;
        using DstLimits = NarrowingRange<Dst, Src, Bounds>;
        using Promotion = decltype(Src() + Dst());
        bool ge_zero = false;
        // Converting floating-point to integer will discard fractional part, so
        // values in (-1.0, -0.0) will truncate to 0 and fit in Dst.
        if (std::is_floating_point_v<Src>) {
            ge_zero = value > Src(-1);
        } else {
            ge_zero = value >= Src(0);
        }
        return RangeCheck(
            ge_zero &&
                (DstLimits::lowest() == 0 || static_cast<Dst>(value) >= DstLimits::lowest()),
            static_cast<Promotion>(SrcLimits::max()) <= static_cast<Promotion>(DstLimits::max()) ||
                static_cast<Promotion>(value) <= static_cast<Promotion>(DstLimits::max()));
    }
};

template <typename Dst, template <typename> class Bounds = std::numeric_limits, typename Src>
constexpr RangeCheck DstRangeRelationToSrcRange(Src value) {
    static_assert(std::is_arithmetic_v<Src>, "Argument must be numeric.");
    static_assert(std::is_arithmetic_v<Dst>, "Result must be numeric.");
    static_assert(Bounds<Dst>::lowest() < Bounds<Dst>::max(), "");
    return DstRangeRelationToSrcRangeImpl<Dst, Src, Bounds>::Check(value);
}

// Extracts the underlying type from an enum.
template <typename T, bool is_enum = std::is_enum_v<T>>
struct ArithmeticOrUnderlyingEnum;

template <typename T>
struct ArithmeticOrUnderlyingEnum<T, true> {
    using type = typename std::underlying_type<T>::type;
    static const bool value = std::is_arithmetic_v<type>;
};

template <typename T>
struct ArithmeticOrUnderlyingEnum<T, false> {
    using type = T;
    static const bool value = std::is_arithmetic_v<type>;
};

// The following are helper templates used in the CheckedNumeric class.
template <typename T>
class CheckedNumeric;

template <typename T>
class ClampedNumeric;

template <typename T>
class StrictNumeric;

// Used to treat CheckedNumeric and arithmetic underlying types the same.
template <typename T>
struct UnderlyingType {
    using type = typename ArithmeticOrUnderlyingEnum<T>::type;
    static const bool is_numeric = std::is_arithmetic_v<type>;
    static const bool is_checked = false;
    static const bool is_clamped = false;
    static const bool is_strict = false;
};

template <typename T>
struct UnderlyingType<CheckedNumeric<T>> {
    using type = T;
    static const bool is_numeric = true;
    static const bool is_checked = true;
    static const bool is_clamped = false;
    static const bool is_strict = false;
};

template <typename T>
struct UnderlyingType<ClampedNumeric<T>> {
    using type = T;
    static const bool is_numeric = true;
    static const bool is_checked = false;
    static const bool is_clamped = true;
    static const bool is_strict = false;
};

template <typename T>
struct UnderlyingType<StrictNumeric<T>> {
    using type = T;
    static const bool is_numeric = true;
    static const bool is_checked = false;
    static const bool is_clamped = false;
    static const bool is_strict = true;
};

// as_signed<> returns the supplied integral value (or integral castable
// Numeric template) cast as a signed integral of equivalent precision.
// I.e. it's mostly an alias for: static_cast<std::make_signed<T>::type>(t)
template <typename Src>
constexpr typename std::make_signed<typename base::internal::UnderlyingType<Src>::type>::type
as_signed(const Src value) {
    static_assert(std::is_integral_v<decltype(as_signed(value))>,
                  "Argument must be a signed or unsigned integer type.");
    return static_cast<decltype(as_signed(value))>(value);
}

// as_unsigned<> returns the supplied integral value (or integral castable
// Numeric template) cast as an unsigned integral of equivalent precision.
// I.e. it's mostly an alias for: static_cast<std::make_unsigned<T>::type>(t)
template <typename Src>
constexpr typename std::make_unsigned<typename base::internal::UnderlyingType<Src>::type>::type
as_unsigned(const Src value) {
    static_assert(std::is_integral_v<decltype(as_unsigned(value))>,
                  "Argument must be a signed or unsigned integer type.");
    return static_cast<decltype(as_unsigned(value))>(value);
}

template <typename L, typename R>
constexpr bool IsGreaterOrEqualImpl(const L lhs,
                                    const R rhs,
                                    const RangeCheck l_range,
                                    const RangeCheck r_range) {
    return l_range.IsOverflow() || r_range.IsUnderflow() ||
           (l_range == r_range &&
            static_cast<decltype(lhs + rhs)>(lhs) >= static_cast<decltype(lhs + rhs)>(rhs));
}

template <typename L, typename R>
struct IsGreaterOrEqual {
    static_assert(std::is_arithmetic_v<L> && std::is_arithmetic_v<R>, "Types must be numeric.");
    static constexpr bool Test(const L lhs, const R rhs) {
        return IsGreaterOrEqualImpl(lhs, rhs, DstRangeRelationToSrcRange<R>(lhs),
                                    DstRangeRelationToSrcRange<L>(rhs));
    }
};

template <typename Dst, typename Src>
constexpr bool IsMaxInRangeForNumericType() {
    return IsGreaterOrEqual<Dst, Src>::Test(std::numeric_limits<Dst>::max(),
                                            std::numeric_limits<Src>::max());
}

template <typename Dst, typename Src>
constexpr Dst CommonMax() {
    return !IsMaxInRangeForNumericType<Dst, Src>() ? Dst(std::numeric_limits<Dst>::max())
                                                   : Dst(std::numeric_limits<Src>::max());
}

}  // namespace base::internal
