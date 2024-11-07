#pragma once

#include <format>

namespace app {

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
};

}  // namespace app

// template <>
// struct std::formatter<app::Key> {
//     constexpr auto parse(auto& ctx) {
//         return ctx.begin();
//     }

//     auto format(const auto& key, auto& ctx) const {
//         static constexpr struct {
//             app::Key key;
//             std::string str;
//         } kKeyMap[] = {
//             {app::Key::kNone, "None"},
//             {app::Key::kA, "A"},
//             {app::Key::kB, "B"},
//             {app::Key::kC, "C"},
//             {app::Key::kD, "D"},
//             {app::Key::kE, "E"},
//             {app::Key::kF, "F"},
//             {app::Key::kG, "G"},
//             {app::Key::kH, "H"},
//             {app::Key::kI, "I"},
//             {app::Key::kJ, "J"},
//             {app::Key::kK, "K"},
//             {app::Key::kL, "L"},
//             {app::Key::kM, "M"},
//             {app::Key::kN, "N"},
//             {app::Key::kO, "O"},
//             {app::Key::kP, "P"},
//             {app::Key::kQ, "Q"},
//             {app::Key::kR, "R"},
//             {app::Key::kS, "S"},
//             {app::Key::kT, "T"},
//             {app::Key::kU, "U"},
//             {app::Key::kV, "V"},
//             {app::Key::kW, "W"},
//             {app::Key::kX, "X"},
//             {app::Key::kY, "Y"},
//             {app::Key::kZ, "Z"},
//             {app::Key::k0, "0"},
//             {app::Key::k1, "1"},
//             {app::Key::k2, "2"},
//             {app::Key::k3, "3"},
//             {app::Key::k4, "4"},
//             {app::Key::k5, "5"},
//             {app::Key::k6, "6"},
//             {app::Key::k7, "7"},
//             {app::Key::k8, "8"},
//             {app::Key::k9, "9"},
//             {app::Key::kEnter, "Enter"},
//             {app::Key::kBackspace, "Backspace"},
//             {app::Key::kTab, "Tab"},
//             {app::Key::kLeftArrow, "LeftArrow"},
//             {app::Key::kRightArrow, "RightArrow"},
//             {app::Key::kDownArrow, "DownArrow"},
//             {app::Key::kUpArrow, "UpArrow"},
//         };
//         for (size_t i = 0; i < std::size(kKeyMap); ++i) {
//             if (kKeyMap[i].key == key) {
//                 return std::format_to(ctx.out(), "Key({})", kKeyMap[i].str);
//             }
//         }
//         return std::format_to(ctx.out(), "Key(Unknown)");
//     }
// };
