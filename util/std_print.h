#pragma once

#include <cstdio>
#include <format>
#include <iostream>
#include <string_view>

// TODO: Once C++23 is released, along with std::print(), this code
// will become obsolete.

namespace std {

// DEV: Default to stderr instead of stdout.
template <typename... Args>
constexpr void print(const std::string_view str_fmt, Args&&... args) {
    fputs(std::vformat(str_fmt, std::make_format_args(args...)).c_str(), stderr);
}

template <typename... Args>
constexpr void print(FILE* fdest, const std::string_view str_fmt, Args&&... args) {
    fputs(std::vformat(str_fmt, std::make_format_args(args...)).c_str(), fdest);
}

template <typename... Args>
constexpr void print(std::ostream& ostream_dest, const std::string_view str_fmt, Args&&... args) {
    ostream_dest << std::vformat(str_fmt, std::make_format_args(args...));
}

// DEV: Default to stderr instead of stdout.
template <typename... Args>
constexpr void println(const std::string_view str_fmt, Args&&... args) {
    fputs(std::vformat(str_fmt, std::make_format_args(args...)).c_str(), stderr);
    fputs("\n", stderr);
}

template <typename... Args>
constexpr void println(FILE* fdest, const std::string_view str_fmt, Args&&... args) {
    fputs(std::vformat(str_fmt, std::make_format_args(args...)).c_str(), fdest);
    fputs("\n", fdest);
}

template <typename... Args>
constexpr void println(std::ostream& ostream_dest,
                       const std::string_view str_fmt,
                       Args&&... args) {
    ostream_dest << std::vformat(str_fmt, std::make_format_args(args...)) << '\n';
}

}
