#pragma once

#include <string>
#include <string_view>

namespace arkan::thanatos::shared::terminal
{
// Strip ANSI CSI sequences roughly: \x1b[ ... 'm'.
// ANSIシーケンス（ざっくり）を取り除く。
inline std::string strip_ansi(std::string_view s)
{
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i)
    {
        if (s[i] == '\x1b')
        {
            // skip until 'm'
            while (i < s.size() && s[i] != 'm') ++i;
            continue;
        }
        out.push_back(s[i]);
    }
    return out;
}

// Visible width of a UTF-8 string in terminal columns (simple).
// 端末の見かけ幅（簡易版）。全角幅は未対応の簡易実装。
inline size_t visible_width_utf8(std::string_view s)
{
    // Very simple: count bytes, assume 1 col per code point.
    // 実運用で拡張するなら、EastAsianWidth を考慮する。
    // Here we naïvely count non-continuation bytes.
    size_t cols = 0;
    for (size_t i = 0; i < s.size(); ++i)
    {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if ((c & 0xC0) != 0x80) ++cols;  // count head bytes
    }
    return cols;
}
}  // namespace arkan::thanatos::shared::terminal
