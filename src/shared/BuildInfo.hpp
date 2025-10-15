#pragma once

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "shared/terminal/Ansi.hpp"
#include "shared/terminal/TextWidth.hpp"
#include "shared/terminal/Utf8.hpp"

namespace arkan::thanatos::shared
{
namespace term = arkan::thanatos::shared::terminal;

inline constexpr std::string_view kProjectName = "Thanatos";
inline constexpr std::string_view kVersion = "0.1.3";
inline constexpr std::string_view kBuildProfile =
#if defined(NDEBUG)
    "Release";
#else
    "Debug";
#endif

// Render signature card and return {blob, inner_width}.
// シグネチャカードを描画して {文字列, 内部幅} を返す。
inline std::pair<std::string, int> build_signature_footer_with_width(bool color = true,
                                                                     bool utf8Box = true)
{
    using namespace std::string_literals;

    // Colors
    const auto B = color ? std::string(ansi::bold) : ""s;
    const auto D = color ? std::string(ansi::dim) : ""s;
    const auto R = color ? std::string(ansi::red) : ""s;
    const auto G = color ? std::string(ansi::green) : ""s;
    const auto C = color ? std::string(ansi::cyan) : ""s;
    const auto Y = color ? std::string(ansi::yellow) : ""s;
    const auto M = color ? std::string(ansi::mag) : ""s;
    const auto W = color ? std::string(ansi::white) : ""s;
    const auto RS = color ? std::string(ansi::reset) : ""s;

    // Lines (content only).
    const auto title = B + R + std::string(kProjectName) + RS + " " + D + "(" +
                       std::string(kBuildProfile) + ")" + RS;
    const auto tagline = "⚔  Forged in chaos. Delivered with precision.";
    const auto vline = C + "Version" + RS + ": " + B + std::string(kVersion) + RS;
    const auto auth = C + "Author " + RS + ": " + "Rhayden – Arkan Software";
    const auto mail = C + "Email  " + RS + ": " + Y + "Rhayden@arkansoftware.com" + RS;
    const auto git =
        C + "Github " + RS + ": " + M + "https://github.com/MarcoRhayden/thanatos" + RS;

    std::vector<std::string> content{title, tagline, vline, auth, mail, git};
    int inner = 0;
    for (auto& s : content)
        inner = std::max(inner, static_cast<int>(term::visible_width_utf8(term::strip_ansi(s))));

    const int pad_lr = 2;
    const int box_w = inner + pad_lr * 2;

    auto padLine = [&](const std::string& raw)
    {
        int vis = static_cast<int>(term::visible_width_utf8(term::strip_ansi(raw)));
        int pad = std::max(0, inner - vis);
        return std::string(pad_lr, ' ') + raw + std::string(pad, ' ') + std::string(pad_lr, ' ');
    };

    std::string out;
    out.reserve(256);
    out += "\n";

    if (utf8Box)
    {
        const std::string top = "┌" + term::repeat_utf8("─", box_w) + "┐\n";
        const std::string sep = "├" + term::repeat_utf8("─", box_w) + "┤\n";
        const std::string bot = "└" + term::repeat_utf8("─", box_w) + "┘\n";

        out += top;
        out += "│" + padLine(W + title + RS) + "│\n";
        out += "│" + padLine(std::string(ansi::dim) + tagline + RS) + "│\n";
        out += sep;
        out += "│" + padLine(vline) + "│\n";
        out += "│" + padLine(auth) + "│\n";
        out += "│" + padLine(mail) + "│\n";
        out += "│" + padLine(git) + "│\n";
        out += bot;
    }
    else
    {
        const std::string top = "+" + std::string(box_w, '-') + "+\n";
        const std::string sep = "+" + std::string(box_w, '-') + "+\n";
        const std::string bot = "+" + std::string(box_w, '-') + "+\n";

        out += top;
        out += "|" + padLine(W + title + RS) + "|\n";
        out += "|" + padLine(std::string(ansi::dim) + tagline + RS) + "|\n";
        out += sep;
        out += "|" + padLine(vline) + "|\n";
        out += "|" + padLine(auth) + "|\n";
        out += "|" + padLine(mail) + "|\n";
        out += "|" + padLine(git) + "|\n";
        out += bot;
    }

    out += "\n";
    return {out, inner};
}

// Keep old API for compatibility.
// 互換性のため旧APIも提供。
inline std::string build_signature_footer(bool color = true, bool utf8Box = true)
{
    return build_signature_footer_with_width(color, utf8Box).first;
}
}  // namespace arkan::thanatos::shared
