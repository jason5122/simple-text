#pragma once

#include "build/buildflag.h"
#include "gui/bitmask_enum.h"

namespace gui {

enum class ModifierKey {
    kNone = 0,
    kShift = 1 << 0,
    kControl = 1 << 1,
    kAlt = 1 << 2,
    kSuper = 1 << 3,
};

template <> struct is_bitmask_enum<gui::ModifierKey> : std::true_type {};

#if IS_MAC
constexpr ModifierKey kPrimaryModifier = ModifierKey::kSuper;
#else
constexpr ModifierKey kPrimaryModifier = ModifierKey::kControl;
#endif

}