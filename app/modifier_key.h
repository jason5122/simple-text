#pragma once

#include "build/build_config.h"
#include <format>

namespace app {

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

inline constexpr auto& operator|=(ModifierKey& l, ModifierKey r) {
    return l = l | r;
}

inline constexpr auto operator&(ModifierKey l, ModifierKey r) {
    using U = std::underlying_type_t<ModifierKey>;
    return static_cast<ModifierKey>(static_cast<U>(l) & static_cast<U>(r));
}

inline constexpr auto& operator&=(ModifierKey& l, ModifierKey r) {
    return l = l & r;
}

inline constexpr auto operator^(ModifierKey l, ModifierKey r) {
    using U = std::underlying_type_t<ModifierKey>;
    return static_cast<ModifierKey>(static_cast<U>(l) ^ static_cast<U>(r));
}

inline constexpr auto& operator^=(ModifierKey& l, ModifierKey r) {
    return l = l ^ r;
}

inline constexpr auto operator~(ModifierKey m) {
    using U = std::underlying_type_t<ModifierKey>;
    return static_cast<ModifierKey>(~static_cast<U>(m));
}

}  // namespace app

template <>
struct std::formatter<app::ModifierKey> {
    constexpr auto parse(auto& ctx) {
        return ctx.begin();
    }

    auto format(const auto& modifiers, auto& ctx) const {
        std::string modifiers_str;
        if ((modifiers & app::ModifierKey::kShift) != app::ModifierKey::kNone) {
            modifiers_str += "Shift";
        }
        if ((modifiers & app::ModifierKey::kControl) != app::ModifierKey::kNone) {
            modifiers_str += "Control";
        }
        if ((modifiers & app::ModifierKey::kAlt) != app::ModifierKey::kNone) {
            modifiers_str += "Alt";
        }
        if ((modifiers & app::ModifierKey::kSuper) != app::ModifierKey::kNone) {
            modifiers_str += "Super";
        }
        return std::format_to(ctx.out(), "ModifierKey({})", modifiers_str);
    }
};
