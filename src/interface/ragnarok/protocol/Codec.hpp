#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace arkan::thanatos::interface::ro::protocol
{

// ====================== Packet & helpers ======================
// Minimal byte buffer used for RO wire encoding.
// ROワイヤエンコード用の最小限なバイトバッファ。
using Packet = std::vector<uint8_t>;

/// Read a 16-bit little-endian value from raw bytes.
/// 生バイト列から16ビットのリトルエンディアン値を読み取ります。
inline std::uint16_t rd16le(const std::uint8_t* p) noexcept
{
    // Expect p to point to at least 2 bytes.
    // pは最低2バイトを指している前提です。
    return static_cast<uint16_t>(p[0] | (static_cast<uint16_t>(p[1]) << 8));
}

/// Append a 16-bit value in little-endian to the packet.
/// 16ビット値をリトルエンディアンでパケット末尾に追加します。
inline void put16(Packet& p, uint16_t v)
{
    p.push_back(static_cast<uint8_t>(v & 0xFF));
    p.push_back(static_cast<uint8_t>(v >> 8));
}

/// Append a 32-bit value in little-endian to the packet.
/// 32ビット値をリトルエンディアンでパケット末尾に追加します。
inline void put32(Packet& p, uint32_t v)
{
    for (int i = 0; i < 4; ++i) p.push_back(static_cast<uint8_t>((v >> (8 * i)) & 0xFF));
}

/// Append a fixed-size, NUL-padded string (max bytes = max).
/// 固定長のNULパディング文字列を追加（最大バイト数 = max）。
inline void putZ(Packet& p, std::string_view s, size_t max)
{
    // Writes up to `max` bytes and pads the rest with 0x00.
    // 最大`max`バイトまで書き、残りを0x00で埋めます。
    for (size_t i = 0; i < max; ++i) p.push_back(i < s.size() ? static_cast<uint8_t>(s[i]) : 0);
}

/// Append an arbitrary fixed-size blob from memory.
/// メモリ上の任意の固定長ブロブを追加します。
inline void putFixed(Packet& p, const void* ptr, size_t n)
{
    const auto* b = static_cast<const uint8_t*>(ptr);
    p.insert(p.end(), b, b + n);
}

// A3 coordinate packing: x(10) | y(10) | dir(4), little-endian 3 bytes.
// A3座標のパック方式: x(10) | y(10) | dir(4) をLE 3バイトで格納。
inline void putCoordsA3(Packet& p, uint16_t x, uint16_t y, uint8_t dir)
{
    // x,y are masked to 10 bits; dir is masked to 4 bits.
    // x,yは10ビットに、dirは4ビットにマスクします。
    const uint32_t enc = (static_cast<uint32_t>(x & 0x03FF)) |
                         (static_cast<uint32_t>(y & 0x03FF) << 10) |
                         (static_cast<uint32_t>(dir & 0x0F) << 20);
    p.push_back(static_cast<uint8_t>(enc & 0xFF));
    p.push_back(static_cast<uint8_t>((enc >> 8) & 0xFF));
    p.push_back(static_cast<uint8_t>((enc >> 16) & 0xFF));
}

// Duplicates kept for compatibility with older call sites.
// 既存呼び出し互換のための重複ヘルパ。
inline void wr16le(std::vector<uint8_t>& out, uint16_t v)
{
    out.push_back(static_cast<uint8_t>(v & 0xFF));
    out.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
}

inline void wr32le(std::vector<uint8_t>& out, uint32_t v)
{
    out.push_back(static_cast<uint8_t>(v & 0xFF));
    out.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
    out.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
    out.push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
}

inline void put16_at(std::vector<uint8_t>& buf, size_t at, uint16_t v)
{
    buf.at(at + 0) = static_cast<uint8_t>(v & 0xFF);
    buf.at(at + 1) = static_cast<uint8_t>((v >> 8) & 0xFF);
}

inline void put32_at(std::vector<uint8_t>& buf, size_t at, uint32_t v)
{
    buf.at(at + 0) = static_cast<uint8_t>(v & 0xFF);
    buf.at(at + 1) = static_cast<uint8_t>((v >> 8) & 0xFF);
    buf.at(at + 2) = static_cast<uint8_t>((v >> 16) & 0xFF);
    buf.at(at + 3) = static_cast<uint8_t>((v >> 24) & 0xFF);
}

inline void putZ_at(std::vector<uint8_t>& buf, size_t at, std::string_view s, size_t max)
{
    for (size_t i = 0; i < max; ++i)
        buf.at(at + i) = (i < s.size()) ? static_cast<uint8_t>(s[i]) : 0u;
}

/// Monotonic millisecond tick for packet timestamps/anti-replay.
/// パケットのタイムスタンプ/再送対策に使う単調増加ミリ秒カウンタ。
uint32_t tick_ms();

}  // namespace arkan::thanatos::interface::ro::protocol
