#pragma once

#include "build/build_config.h"
#include <type_traits>

namespace gui {

// TODO: Add all keys from: https://www.sublimetext.com/docs/key_bindings.html#key-names
enum class Key {
    kNone,
    kA,
    kB,
    kC,
    kD,
    kE,
    kF,
    kG,
    kH,
    kI,
    kJ,
    kK,
    kL,
    kM,
    kN,
    kO,
    kP,
    kQ,
    kR,
    kS,
    kT,
    kU,
    kV,
    kW,
    kX,
    kY,
    kZ,
    k0,
    k1,
    k2,
    k3,
    k4,
    k5,
    k6,
    k7,
    k8,
    k9,
    kEnter,
    kBackspace,
    kTab,
    kLeftArrow,
    kRightArrow,
    kDownArrow,
    kUpArrow,
    kEqual,
    kMinus,
};

enum class ModifierKey {
    kNone = 0,
    kShift = 1 << 0,
    kControl = 1 << 1,
    kAlt = 1 << 2,
    kSuper = 1 << 3,
};

#if BUILDFLAG(IS_MAC)
constexpr ModifierKey kPrimaryModifier = ModifierKey::kSuper;
#else
constexpr ModifierKey kPrimaryModifier = ModifierKey::kControl;
#endif

inline constexpr auto operator|(ModifierKey l, ModifierKey r) {
    using U = std::underlying_type_t<ModifierKey>;
    return static_cast<ModifierKey>(static_cast<U>(l) | static_cast<U>(r));
}

inline constexpr auto& operator|=(ModifierKey& l, ModifierKey r) { return l = l | r; }

inline constexpr auto operator&(ModifierKey l, ModifierKey r) {
    using U = std::underlying_type_t<ModifierKey>;
    return static_cast<ModifierKey>(static_cast<U>(l) & static_cast<U>(r));
}

inline constexpr auto& operator&=(ModifierKey& l, ModifierKey r) { return l = l & r; }

inline constexpr auto operator^(ModifierKey l, ModifierKey r) {
    using U = std::underlying_type_t<ModifierKey>;
    return static_cast<ModifierKey>(static_cast<U>(l) ^ static_cast<U>(r));
}

inline constexpr auto& operator^=(ModifierKey& l, ModifierKey r) { return l = l ^ r; }

inline constexpr auto operator~(ModifierKey m) {
    using U = std::underlying_type_t<ModifierKey>;
    return static_cast<ModifierKey>(~static_cast<U>(m));
}

// TODO: Fully implement this list.
constexpr bool CanBeChar(Key key) {
    switch (key) {
    case Key::kA:
    case Key::kB:
    case Key::kC:
    case Key::kD:
    case Key::kE:
    case Key::kF:
    case Key::kG:
    case Key::kH:
    case Key::kI:
    case Key::kJ:
    case Key::kK:
    case Key::kL:
    case Key::kM:
    case Key::kN:
    case Key::kO:
    case Key::kP:
    case Key::kQ:
    case Key::kR:
    case Key::kS:
    case Key::kT:
    case Key::kU:
    case Key::kV:
    case Key::kW:
    case Key::kX:
    case Key::kY:
    case Key::kZ:
    case Key::k0:
    case Key::k1:
    case Key::k2:
    case Key::k3:
    case Key::k4:
    case Key::k5:
    case Key::k6:
    case Key::k7:
    case Key::k8:
    case Key::k9:
        return true;
    default:
        return false;
    }
}

}  // namespace gui
