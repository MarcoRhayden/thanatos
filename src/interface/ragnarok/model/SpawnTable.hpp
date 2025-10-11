#pragma once

#include <cstdint>
#include <string>
#include <tuple>
#include <unordered_map>

namespace arkan::thanatos::interface::ro::model
{

struct Spawn
{
    uint16_t x, y;
    uint8_t dir;
};

inline Spawn safeSpawnFor(const std::string& map)
{
    static const std::unordered_map<std::string, Spawn> k{{"lhz_dun_n.gat", {263, 139, 0}}};

    if (auto it = k.find(map); it != k.end()) return it->second;

    return {263, 139, 0};
}

}  // namespace arkan::thanatos::interface::ro::model
