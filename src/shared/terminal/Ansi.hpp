#pragma once
#include <string_view>

namespace arkan::thanatos::shared::ansi
{
// Basic ANSI SGR tokens.
// 端末のANSIカラー用トークン。
inline constexpr std::string_view reset = "\x1b[0m";
inline constexpr std::string_view bold = "\x1b[1m";
inline constexpr std::string_view dim = "\x1b[2m";

inline constexpr std::string_view red = "\x1b[31m";
inline constexpr std::string_view green = "\x1b[32m";
inline constexpr std::string_view yellow = "\x1b[33m";
inline constexpr std::string_view blue = "\x1b[34m";
inline constexpr std::string_view mag = "\x1b[35m";
inline constexpr std::string_view cyan = "\x1b[36m";
inline constexpr std::string_view white = "\x1b[37m";
}  // namespace arkan::thanatos::shared::ansi
