#pragma once

#include <cstdint>

namespace arkan::thanatos::interface::ro::protocol
{

// ====================== A3 packed coordinates ======================
// 10-bit X, 10-bit Y, 4-bit facing direction packed into 3 bytes (LE).
// 3バイト（LE）に X(10)・Y(10)・方向(4) を詰め込むA3形式。
struct A3
{
    uint16_t x;   // 0..1023 tile X / タイルX（0..1023）
    uint16_t y;   // 0..1023 tile Y / タイルY（0..1023）
    uint8_t dir;  // 0..15  facing / 向き（0..15）
};

// data[0..2] encodes A3 as: x(10) | y(10) | dir(4), little-endian bytes.
// data[0..2] は A3 を x(10) | y(10) | dir(4) のLEバイトで表現します。
inline A3 decodeA3(const uint8_t* p) noexcept
{
    // Expect p to reference at least 3 bytes.
    // p は最低3バイトを指している前提です。
    const uint32_t enc = static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
                         (static_cast<uint32_t>(p[2]) << 16);

    // Mask to the intended bit widths.
    // それぞれのビット幅にマスクします。
    A3 out{
        static_cast<uint16_t>(enc & 0x03FF),          // x: 10 bits / 10ビット
        static_cast<uint16_t>((enc >> 10) & 0x03FF),  // y: 10 bits / 10ビット
        static_cast<uint8_t>((enc >> 20) & 0x0F)      // dir: 4 bits / 4ビット
    };
    return out;
}

// Clamp x/y to 10-bit range (0..1023) and dir to 4-bit (0..15).
// x/y を10ビット範囲(0..1023)、dir を4ビット範囲(0..15)に丸めます。
inline A3 clamp1023(const A3& a) noexcept
{
    A3 r = a;
    if (r.x > 1023) r.x = 1023;  // prevent overflow in packing / パック時のオーバーフロー防止
    if (r.y > 1023) r.y = 1023;
    r.dir &= 0x0F;  // keep lower 4 bits / 下位4ビットのみ維持
    return r;
}

}  // namespace arkan::thanatos::interface::ro::protocol
