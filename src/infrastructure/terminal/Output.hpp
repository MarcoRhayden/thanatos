#pragma once

#include <cstdio>
#include <string>
#include <string_view>

#if defined(_WIN32)
#include "shared/BannerPrinter.hpp"
#endif

namespace arkan::thanatos::infrastructure::terminal
{
// Emit a raw UTF-8 line with platform-friendly newline.
// プラットフォーム互換の改行付きでUTF-8行を出力。
inline void emit_utf8_line(std::string_view utf8)
{
#if defined(_WIN32)
    arkan::thanatos::shared::print_utf8_banner(std::string(utf8));
    arkan::thanatos::shared::print_utf8_banner(std::string("\r\n"));
#else
    std::fwrite(utf8.data(), 1, utf8.size(), stdout);
    std::fwrite("\n", 1, 1, stdout);
#endif
}

// Emit a blank line.
// 空行を出力。
inline void emit_blank_line()
{
    std::fwrite("\n", 1, 1, stdout);
}

// Print a whole blob as-is (no extra newline).
// 生文字列をそのまま出力（改行追加なし）。
inline void print_portable(std::string_view blob)
{
#if defined(_WIN32)
    arkan::thanatos::shared::print_utf8_banner(std::string(blob));
#else
    std::fwrite(blob.data(), 1, blob.size(), stdout);
#endif
}
}  // namespace arkan::thanatos::infrastructure::terminal
