#pragma once

#include <cstdint>

namespace arkan::poseidon::interface::ro::proto
{

struct A3
{
    uint16_t x;
    uint16_t y;
    uint8_t dir;  // 0..15
};

// data[0..2] = A3 LE: x(10) | y(10) | dir(4)
inline A3 decodeA3(const uint8_t* p)
{
    const uint32_t enc = (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16);
    return A3{(uint16_t)(enc & 0x03FF), (uint16_t)((enc >> 10) & 0x03FF),
              (uint8_t)((enc >> 20) & 0x0F)};
}

inline A3 clamp1023(const A3& a)
{
    A3 r = a;
    if (r.x > 1023) r.x = 1023;
    if (r.y > 1023) r.y = 1023;
    r.dir &= 0x0F;
    return r;
}

}  // namespace arkan::poseidon::interface::ro::proto
