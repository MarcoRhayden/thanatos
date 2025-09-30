#pragma once

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

namespace arkan::poseidon::shared::hex
{

// -----------------------------------------------------------------------------
// Helpers to parse/format binary buffers as hexadecimal text.
// Design goals:
//  - Header-only, zero-allocation APIs where possible
//  - Safe casting (unsigned char) for <cctype> calls
//  - Tolerant parser: ignores any non-hex characters (spaces, commas, "0x", etc.)
//  - Tiny convenience formatters for 16/32-bit integers and 4-byte arrays
// -----------------------------------------------------------------------------

namespace detail
{
[[nodiscard]] inline int hexval(char c) noexcept
{
    // Normalize to unsigned to avoid UB with negative chars
    const unsigned char uc = static_cast<unsigned char>(c);

    if (uc >= '0' && uc <= '9') return static_cast<int>(uc - '0');
    if (uc >= 'a' && uc <= 'f') return static_cast<int>(uc - 'a' + 10);
    if (uc >= 'A' && uc <= 'F') return static_cast<int>(uc - 'A' + 10);
    return -1;
}
}  // namespace detail

// Parse a hex string like any of the following forms:
//   "69 00 00 00"
//   "0x69,0x00,0x00,0x00"
//   "69000000"
//   "69-00-00-00"
// Non-hex characters are ignored. Returns true on success.
[[nodiscard]] inline bool parse_hex_string(std::string_view s, std::vector<std::uint8_t>& out)
{
    std::string flat;
    flat.reserve(s.size());
    for (char c : s)
    {
        if (std::isxdigit(static_cast<unsigned char>(c))) flat.push_back(c);
    }
    if (flat.size() % 2 != 0) return false;

    out.clear();
    out.reserve(flat.size() / 2);

    for (size_t i = 0; i < flat.size(); i += 2)
    {
        const int hi = detail::hexval(flat[i]);
        const int lo = detail::hexval(flat[i + 1]);
        if (hi < 0 || lo < 0) return false;
        out.push_back(static_cast<std::uint8_t>((hi << 4) | lo));
    }
    return true;
}

// Hex-dump an arbitrary contiguous buffer into a string like "69 00 00 00".
[[nodiscard]] inline std::string hex(const uint8_t* p, size_t n)
{
    std::string s;
    s.reserve(n * 3);
    for (size_t i = 0; i < n; ++i)
    {
        char b[4];
        std::snprintf(b, sizeof(b), "%02X", p[i]);
        if (i) s.push_back(' ');
        s += b;
    }
    return s;
}

// Convenience for 4-byte arrays (e.g., IPv4, IDs)
[[nodiscard]] inline std::string hex4(const std::array<uint8_t, 4>& a)
{
    return hex(a.data(), a.size());
}

// Format 16/32-bit little-endian integers as "XXXX" / "XXXXXXXX"
[[nodiscard]] inline std::string hex16(uint16_t v)
{
    char b[6];
    std::snprintf(b, sizeof(b), "%04X", v);
    return b;
}

[[nodiscard]] inline std::string hex32(uint32_t v)
{
    char b[10];
    std::snprintf(b, sizeof(b), "%08X", v);
    return b;
}

}  // namespace arkan::poseidon::shared::hex
