#pragma once
#include <cstdio>
#include <string>
#include <string_view>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <langinfo.h>
#include <unistd.h>

#include <clocale>
#endif

namespace arkan::thanatos::shared
{

// Fallback ASCII for environments without UTF-8 / without braille glyphs
inline constexpr std::string_view kAsciiFallback =
    R"(===============================
  Thanatos - Arkan Software
===============================
)";

#if defined(_WIN32)
inline std::wstring utf8_to_utf16(std::string_view s)
{
    if (s.empty()) return {};
    int len = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
    std::wstring w(static_cast<size_t>(len), L'\0');
    (void)MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), w.data(), len);
    return w;
}

inline bool is_console(HANDLE h)
{
    DWORD mode;
    return (h != INVALID_HANDLE_VALUE) && GetConsoleMode(h, &mode);
}

inline void enable_vt_utf8(HANDLE h)
{
    SetConsoleOutputCP(CP_UTF8);
    DWORD mode = 0;
    if (GetConsoleMode(h, &mode))
    {
        mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(h, mode);
    }
}

inline void print_utf8_banner(std::string_view utf8)
{
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (is_console(h))
    {
        enable_vt_utf8(h);
        auto w = utf8_to_utf16(utf8);
        DWORD written = 0;
        WriteConsoleW(h, w.c_str(), static_cast<DWORD>(w.size()), &written, nullptr);
        return;
    }
    std::fwrite(utf8.data(), 1, utf8.size(), stdout);
}
#else
inline bool locale_is_utf8()
{
    std::setlocale(LC_ALL, "");
    const char* cs = nl_langinfo(CODESET);
    return cs && std::string(cs) == "UTF-8";
}

inline void print_utf8_banner(std::string_view utf8)
{
    if (isatty(STDOUT_FILENO) && locale_is_utf8())
    {
        std::fwrite(utf8.data(), 1, utf8.size(), stdout);
    }
    else
    {
        std::fwrite(kAsciiFallback.data(), 1, kAsciiFallback.size(), stdout);
    }
}
#endif

inline void print_banner_or_fallback(std::string_view utf8)
{
#if defined(_WIN32)
    print_utf8_banner(utf8);
#else
    if (locale_is_utf8())
    {
        print_utf8_banner(utf8);
    }
    else
    {
        std::fwrite(kAsciiFallback.data(), 1, kAsciiFallback.size(), stdout);
    }
#endif
}

}  // namespace arkan::thanatos::shared
