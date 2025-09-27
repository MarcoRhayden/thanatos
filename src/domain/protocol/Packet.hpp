#pragma once

#include <cstdint>
#include <vector>

namespace arkan::poseidon::domain::protocol
{

struct Packet
{
    std::uint16_t opcode{0};            // LE
    std::vector<std::uint8_t> payload;  // size = total_size - 4
};

}  // namespace arkan::poseidon::domain::protocol
