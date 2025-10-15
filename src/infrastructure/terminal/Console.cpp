#include "infrastructure/terminal/Console.hpp"

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace arkan::thanatos::infrastructure::terminal
{
bool locale_is_utf8()
{
#if defined(_WIN32)
    // If VT mode is available, we assume the console can show UTF-8.
    // VTモードが使えればUTF-8表示可能とみなす。
    return true;
#else
    // On POSIX we assume UTF-8 locale (customize as needed).
    // POSIXではUTF-8ロケールを仮定（必要なら拡張）。
    return true;
#endif
}

void enable_vt_sequences_if_possible()
{
#if defined(_WIN32)
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;
    DWORD mode = 0;
    if (!GetConsoleMode(hOut, &mode)) return;
    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, mode);
#endif
}
}  // namespace arkan::thanatos::infrastructure::terminal
