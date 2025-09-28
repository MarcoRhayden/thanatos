#pragma once

#include <cctype>
#include <cstdint>
#include <string>
#include <vector>

namespace arkan::poseidon::shared::hex
{

namespace detail
{
inline int hexval(char c)
{
    if (c >= '0' && c <= '9') return (c - '0');
    if (c >= 'a' && c <= 'f') return (c - 'a' + 10);
    if (c >= 'A' && c <= 'F') return (c - 'A' + 10);

    return -1;
}
}  // namespace detail

// Parse a hex string like:
//  "69 00 00 00", "0x69,0x00,0x00,0x00", "69000000", "69-00-00-00"
inline bool parse_hex_string(const std::string& s, std::vector<std::uint8_t>& out)
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

}  // namespace arkan::poseidon::shared::hex
