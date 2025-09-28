#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace arkan::poseidon::domain::protocol
{
struct ServerEntry
{
    std::string name;
    std::string host;
    std::uint16_t port{};
};

struct Character
{
    std::string name;
    std::string map;
    std::uint16_t x{};
    std::uint16_t y{};
};
}  // namespace arkan::poseidon::domain::protocol
