#pragma once

#include "safe_conversions_impl.h"

namespace base {
namespace internal {

// strict_cast<> is analogous to static_cast<> for numeric types, except that
// it will cause a compile failure if the destination type is not large enough
// to contain any value in the source type. It performs no runtime checking.
template <typename Dst, typename Src>
constexpr Dst strict_cast(Src value) {
    using SrcType = typename UnderlyingType<Src>::type;
    static_assert(UnderlyingType<Src>::is_numeric, "Argument must be numeric.");
    static_assert(std::is_arithmetic_v<Dst>, "Result must be numeric.");

    // If you got here from a compiler error, it's because you tried to assign
    // from a source type to a destination type that has insufficient range.
    // The solution may be to change the destination type you're assigning to,
    // and use one large enough to represent the source.
    // Alternatively, you may be better served with the checked_cast<> or
    // saturated_cast<> template functions for your particular use case.
    static_assert(StaticDstRangeRelationToSrcRange<Dst, SrcType>::value == NUMERIC_RANGE_CONTAINED,
                  "The source type is out of range for the destination type. "
                  "Please see strict_cast<> comments for more information.");

    return static_cast<Dst>(static_cast<SrcType>(value));
}

// The following special case a few specific integer conversions where we can
// eke out better performance than range checking.
template <typename Dst, typename Src, typename Enable = void>
struct IsValueInRangeFastOp {
    static constexpr bool is_supported = false;
    static constexpr bool Do(Src value) {
        // Force a compile failure if instantiated.
        return CheckOnFailure::template HandleFailure<bool>();
    }
};

// Signed to signed range comparison.
template <typename Dst, typename Src>
struct IsValueInRangeFastOp<
    Dst,
    Src,
    std::enable_if_t<std::is_integral_v<Dst> && std::is_integral_v<Src> && std::is_signed_v<Dst> &&
                     std::is_signed_v<Src> && !IsTypeInRangeForNumericType<Dst, Src>::value>> {
    static constexpr bool is_supported = true;

    static constexpr bool Do(Src value) {
        // Just downcast to the smaller type, sign extend it back to the original
        // type, and then see if it matches the original value.
        return value == static_cast<Dst>(value);
    }
};

// Signed to unsigned range comparison.
template <typename Dst, typename Src>
struct IsValueInRangeFastOp<Dst,
                            Src,
                            std::enable_if_t<std::is_integral_v<Dst> && std::is_integral_v<Src> &&
                                             !std::is_signed_v<Dst> && std::is_signed_v<Src> &&
                                             !IsTypeInRangeForNumericType<Dst, Src>::value>> {
    static constexpr bool is_supported = true;

    static constexpr bool Do(Src value) {
        // We cast a signed as unsigned to overflow negative values to the top,
        // then compare against whichever maximum is smaller, as our upper bound.
        return as_unsigned(value) <= as_unsigned(CommonMax<Src, Dst>());
    }
};

// Convenience function that returns true if the supplied value is in range
// for the destination type.
template <typename Dst, typename Src>
constexpr bool IsValueInRangeForNumericType(Src value) {
    using SrcType = typename internal::UnderlyingType<Src>::type;
    return internal::IsValueInRangeFastOp<Dst, SrcType>::is_supported
               ? internal::IsValueInRangeFastOp<Dst, SrcType>::Do(static_cast<SrcType>(value))
               : internal::DstRangeRelationToSrcRange<Dst>(static_cast<SrcType>(value)).IsValid();
}

// checked_cast<> is analogous to static_cast<> for numeric types,
// except that it CHECKs that the specified numeric conversion will not
// overflow or underflow. NaN source will always trigger a CHECK.
template <typename Dst, class CheckHandler = internal::CheckOnFailure, typename Src>
constexpr Dst checked_cast(Src value) {
    // This throws a compile-time error on evaluating the constexpr if it can be
    // determined at compile-time as failing, otherwise it will CHECK at runtime.
    using SrcType = typename internal::UnderlyingType<Src>::type;
    return BASE_NUMERICS_LIKELY((IsValueInRangeForNumericType<Dst>(value)))
               ? static_cast<Dst>(static_cast<SrcType>(value))
               : CheckHandler::template HandleFailure<Dst>();
}

}  // namespace internal

using internal::checked_cast;
using internal::strict_cast;

}  // namespace base
