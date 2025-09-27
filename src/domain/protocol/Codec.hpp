#pragma once
#include <cstdint>
#include <vector>

#include "Packet.hpp"

namespace arkan::poseidon::domain::protocol
{

using Buffer = std::vector<std::uint8_t>;

inline void PushLE16(Buffer& b, std::uint16_t v)
{
    b.push_back(static_cast<std::uint8_t>(v & 0xFF));
    b.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
}

// Header: uint16 opcode, uint16 size (header+payload), little-endian.
inline Buffer Encode(const Packet& p)
{
    Buffer out;
    const std::uint16_t total = static_cast<std::uint16_t>(4 + p.payload.size());
    out.reserve(total);
    PushLE16(out, p.opcode);
    PushLE16(out, total);
    out.insert(out.end(), p.payload.begin(), p.payload.end());
    return out;
}

}  // namespace arkan::poseidon::domain::protocol
