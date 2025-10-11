#pragma once

#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

namespace arkan::thanatos::shared::logfmt
{

/**
 * - Prints offset, 16 hex bytes, and their printable ASCII ('.' for control bytes).
 * - Designed for logging binary RO frames in a compact, human-readable form.
 * - No allocations per line other than appending into `out`.
 *
 * - 先頭にオフセット、16バイトの16進表示、右側にASCII（制御文字は '.'）。
 * - RO のバイナリフレームを人間可読に記録する目的。
 * - 1行ごとに新たな動的確保は行わず、出力文字列へ追記のみ。
 */
inline std::string hex_dump(const uint8_t* p, size_t n, size_t bytes_per_line = 16)
{
    std::string out;
    out.reserve(n * 4);  // heuristic: keeps reallocations rare / 再確保を抑制する目安
    char line[128];

    for (size_t off = 0; off < n; off += bytes_per_line)
    {
        // Offset column (zero-padded hex).
        // オフセット（0詰め16進）
        std::snprintf(line, sizeof(line), "%04zx  ", off);
        out += line;

        // Hex bytes + collect ASCII mirror.
        // 16進バイト列 + 右側ASCII欄の収集
        std::string ascii;
        ascii.reserve(bytes_per_line);
        for (size_t i = 0; i < bytes_per_line; ++i)
        {
            if (off + i < n)
            {
                std::snprintf(line, sizeof(line), "%02X ", p[off + i]);
                // Printable? keep; else dot.
                // 表示可能文字ならそのまま、不可なら '.'
                ascii.push_back((p[off + i] >= 0x20 && p[off + i] <= 0x7E) ? char(p[off + i])
                                                                           : '.');
            }
            else
            {
                std::snprintf(line, sizeof(line), "   ");  // pad to keep ASCII aligned / 桁合わせ
                ascii.push_back(' ');
            }
            out += line;
            if (i == 7) out += ' ';  // visual spacer 8+8 / 視認性のため 8 バイト境界でスペース
        }
        out += " | ";
        out += ascii;
        out += '\n';
    }
    return out;
}

/**
 * Describe an RO frame header [u16 opcode][u16 length][payload…] (little-endian).
 * - If `n >= 4`, prints opcode, declared length, and computed payload size.
 * - If truncated, prints a short “incomplete frame” message.
 *
 * RO フレームヘッダ説明子 [u16 opcode][u16 length][payload…]（LE）。
 * - `n >= 4` の場合は opcode/len とペイロード長を表示。
 * - 途切れている場合は「incomplete」を返す。
 */
inline std::string ro_header(const uint8_t* p, size_t n)
{
    auto rd16le = [](const uint8_t* q) -> uint16_t
    { return uint16_t(q[0] | (uint16_t(q[1]) << 8)); };
    char buf[160];
    if (n >= 4)
    {
        const uint16_t op = rd16le(p);
        const uint16_t len = rd16le(p + 2);
        std::snprintf(buf, sizeof(buf), "opcode=0x%04X len=%u payload=%zu", op, len,
                      (len >= 4 && len <= n) ? (size_t(len) - 4) : (n >= 4 ? (n - 4) : 0));
    }
    else
    {
        std::snprintf(buf, sizeof(buf), "incomplete frame: %zu bytes", n);
    }
    return std::string(buf);
}

/**
 * Lightweight banner for log sections (direction + title + size).
 * Example:  "===== TX→CLIENT GG 09CF (80 bytes) ====="
 *
 * ログ用の軽量バナー（方向 + タイトル + サイズ）。
 * 例: "===== TX→CLIENT GG 09CF (80 bytes) ====="
 */
inline std::string banner(std::string_view dir, std::string_view title, size_t total_len)
{
    char head[160];
    std::snprintf(head, sizeof(head), "%s %.*s (%zu bytes)", dir.data(), int(title.size()),
                  title.data(), total_len);
    return std::string(head);
}

}  // namespace arkan::thanatos::shared::logfmt
