#include "infrastructure/presentation/StartupSummary.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>

#include "shared/BannerPrinter.hpp"

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif
#endif

namespace arkan::thanatos::infrastructure::presentation
{

// ---- visible length without ANSI / ANSI を除いた見かけの長さ ----
static std::string strip_ansi(std::string_view s)
{
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i)
    {
        if (s[i] == '\x1b')
        {
            ++i;
            if (i < s.size() && s[i] == '[')
                while (i < s.size() && s[i] != 'm') ++i;
            continue;
        }
        out.push_back(s[i]);
    }
    return out;
}
static inline size_t vis_len(const std::string& s)
{
    return strip_ansi(s).size();
}

// ---- console capability ----
#if defined(_WIN32)
static bool console_utf8_ok()
{
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;
    return (h != INVALID_HANDLE_VALUE) && GetConsoleMode(h, &mode);
}
#else
static bool console_utf8_ok()
{
    // qualify the function from BannerPrinter.hpp
    return arkan::thanatos::shared::locale_is_utf8();
}
#endif

// ---- emit one line (UTF-8 on POSIX, UTF-16 on Win) ----
static void emit_utf8_line(std::string_view utf8)
{
#if defined(_WIN32)
    arkan::thanatos::shared::print_utf8_banner(std::string(utf8));
    arkan::thanatos::shared::print_utf8_banner(std::string("\r\n"));
#else
    std::fwrite(utf8.data(), 1, utf8.size(), stdout);
    std::fwrite("\n", 1, 1, stdout);
#endif
}

// --- repeats a UTF-8 "glyph" n times ---
static std::string repeat_utf8(std::string_view g, size_t n)
{
    std::string out;
    out.reserve(g.size() * n);
    for (size_t i = 0; i < n; ++i) out += g;
    return out;
}

static void emit_ascii_line(std::string_view ascii)
{
    std::fwrite(ascii.data(), 1, ascii.size(), stdout);
    std::fwrite("\n", 1, 1, stdout);
}

void print_startup_summary(const StartupSummary& s)
{
    const bool utf8 = console_utf8_ok();
    const bool color = s.color && utf8;

    // --- Decide frame style (robust default on Windows = ASCII) --------------
#if defined(_WIN32)
    bool fancy_box = false;
#else
    bool fancy_box = true;
#endif
    if (const char* env = std::getenv("THANATOS_UTF8_BOX"))
    {
        if (env[0] == '1') fancy_box = true;
        if (env[0] == '0') fancy_box = false;
    }

    if (s.prefer_utf8_box) fancy_box = true;

    // colors
    const char* BOLD = color ? "\x1b[1m" : "";
    const char* GREEN = color ? "\x1b[32m" : "";
    const char* CYAN = color ? "\x1b[36m" : "";
    const char* YELLOW = color ? "\x1b[33m" : "";
    const char* MAG = color ? "\x1b[35m" : "";
    const char* RESET = color ? "\x1b[0m" : "";

    auto ep = [](const std::string& h, std::uint16_t p) { return h + ":" + std::to_string(p); };

    // lines (content only) / 内容行
    const std::string title = "Thanatos Online";
    auto line_idx = std::string(CYAN) + "Selected port set index" + RESET + " : " + BOLD +
                    std::to_string(s.set_index) + RESET;
    auto line_login = std::string(CYAN) + "RO Login " + RESET + ": " + YELLOW +
                      ep(s.ro_host, s.login_port) + RESET;
    auto line_char = std::string(CYAN) + "RO Char  " + RESET + ": " + YELLOW +
                     ep(s.ro_host, s.char_port) + RESET;
    auto line_bus = std::string(CYAN) + "Query BUS" + RESET + ": " + YELLOW +
                    ep(s.query_host, s.query_port) + RESET;
    auto line_gg = std::string(MAG) + "GG Bridge" + RESET +
                   " : strategy=AUTO, size=[2..4096], timeout=3000ms, greedy=150ms";

    // width / 幅
    size_t inner = 0;
    inner = (std::max)(inner, vis_len(line_idx));
    inner = (std::max)(inner, vis_len(line_login));
    inner = (std::max)(inner, vis_len(line_char));
    inner = (std::max)(inner, vis_len(line_bus));
    inner = (std::max)(inner, vis_len(line_gg));
    inner = (std::max)(inner, title.size() + 2);  // account for the spaces around the title
    const size_t BOX_W = inner + 4;

    if (!fancy_box)
    {
        // ---------------- ASCII frame / ASCII 罫線 ----------------
        auto framed = [&](const std::string& c)
        {
            size_t pad = (vis_len(c) < inner) ? (inner - vis_len(c)) : 0;
            return std::string("| ") + c + std::string(pad, ' ') + " |";
        };

        const std::string top = "+" + std::string(BOX_W - 2, '-') + "+";
        const std::string sep = "+" + std::string(BOX_W - 2, '-') + "+";
        const std::string bot = "+" + std::string(BOX_W - 2, '-') + "+";

        const size_t title_len = title.size() + 2;
        const size_t pad_total = (inner > title_len) ? (inner - title_len) : 0;
        size_t left = pad_total / 2;
        size_t right = pad_total - left;

        std::string ttl = std::string("| ") + GREEN + std::string(left, '=') + " " + title + " " +
                          std::string(right, '=') + RESET + " |";

        emit_ascii_line(top);
        emit_utf8_line(ttl);
        emit_utf8_line(framed(line_idx));
        emit_ascii_line(sep);
        emit_utf8_line(framed(line_login));
        emit_utf8_line(framed(line_char));
        emit_utf8_line(framed(line_bus));
        emit_ascii_line(sep);
        emit_utf8_line(framed(line_gg));
        emit_ascii_line(bot);
        return;
    }

    // ---------------- UTF-8 box-drawing frame / UTF-8 罫線 ----------------
    auto framed = [&](const std::string& c)
    {
        size_t pad = (vis_len(c) < inner) ? (inner - vis_len(c)) : 0;
        return std::string("│ ") + c + std::string(pad, ' ') + " │";
    };

    const std::string top = std::string("┌") + repeat_utf8("─", BOX_W - 2) + std::string("┐");
    const std::string sep = std::string("├") + repeat_utf8("─", BOX_W - 2) + std::string("┤");
    const std::string bot = std::string("└") + repeat_utf8("─", BOX_W - 2) + std::string("┘");

    const size_t title_len = title.size() + 2;
    const size_t pad_total = (inner > title_len) ? (inner - title_len) : 0;
    size_t left = pad_total / 2;
    size_t right = pad_total - left;

    std::string ttl = std::string("│ ") + GREEN + repeat_utf8("─", left) + " " + title + " " +
                      repeat_utf8("─", right) + RESET + std::string(" │");

    emit_utf8_line(top);
    emit_utf8_line(ttl);
    emit_utf8_line(framed(line_idx));
    emit_utf8_line(sep);
    emit_utf8_line(framed(line_login));
    emit_utf8_line(framed(line_char));
    emit_utf8_line(framed(line_bus));
    emit_utf8_line(sep);
    emit_utf8_line(framed(line_gg));
    emit_utf8_line(bot);
}

}  // namespace arkan::thanatos::infrastructure::presentation
