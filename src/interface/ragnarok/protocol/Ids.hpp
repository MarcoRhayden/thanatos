#pragma once

#include <array>
#include <cstdint>

namespace arkan::thanatos::interface::ro::protocol
{

// Little-endian 4-byte wire identities used across Login/Char DTOs and state.
using WireBytes4 = std::array<std::uint8_t, 4>;
using AccountIdLe = WireBytes4;
using SessionIdLe = WireBytes4;
using CharIdLe = WireBytes4;
using Ipv4Octets = WireBytes4;

// Convert a host-endian u32 into the 4-byte little-endian wire form.
inline AccountIdLe to_le_bytes(std::uint32_t value) noexcept
{
    return {
        static_cast<std::uint8_t>(value & 0xFF),
        static_cast<std::uint8_t>((value >> 8) & 0xFF),
        static_cast<std::uint8_t>((value >> 16) & 0xFF),
        static_cast<std::uint8_t>((value >> 24) & 0xFF),
    };
}

}  // namespace arkan::thanatos::interface::ro::protocol
