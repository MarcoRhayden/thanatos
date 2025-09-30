#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace arkan::poseidon::interface::ro::proto
{

using Packet = std::vector<uint8_t>;

inline std::uint16_t rd16le(const std::uint8_t* p)
{
    return uint16_t(p[0] | (p[1] << 8));
}

inline void put16(Packet& p, uint16_t v)
{
    p.push_back(uint8_t(v & 0xFF));
    p.push_back(uint8_t(v >> 8));
}

inline void put32(Packet& p, uint32_t v)
{
    for (int i = 0; i < 4; ++i) p.push_back(uint8_t((v >> (8 * i)) & 0xFF));
}

inline void putZ(Packet& p, std::string_view s, size_t max)
{
    for (size_t i = 0; i < max; ++i) p.push_back(i < s.size() ? uint8_t(s[i]) : 0);
}

inline void putFixed(Packet& p, const void* ptr, size_t n)
{
    auto* b = static_cast<const uint8_t*>(ptr);
    p.insert(p.end(), b, b + n);
}

// A3: x(10) | y(10) | dir(4)  little-endian, 3 bytes
inline void putCoordsA3(Packet& p, uint16_t x, uint16_t y, uint8_t dir)
{
    const uint32_t enc =
        (uint32_t(x & 0x03FF)) | (uint32_t(y & 0x03FF) << 10) | (uint32_t(dir & 0x0F) << 20);
    p.push_back(uint8_t(enc & 0xFF));
    p.push_back(uint8_t((enc >> 8) & 0xFF));
    p.push_back(uint8_t((enc >> 16) & 0xFF));
}

uint32_t tick_ms();

}  // namespace arkan::poseidon::interface::ro::proto
