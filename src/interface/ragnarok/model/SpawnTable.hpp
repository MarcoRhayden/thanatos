#pragma once

#include <cstdint>
#include <string>
#include <tuple>
#include <unordered_map>

namespace arkan::poseidon::interface::ro::model
{

struct Spawn
{
    uint16_t x, y;
    uint8_t dir;
};

inline Spawn safeSpawnFor(const std::string& map)
{
    static const std::unordered_map<std::string, Spawn> k{
        {"prontera.gat", {156, 191, 0}}, {"payon.gat", {181, 108, 2}},
        {"izlude.gat", {128, 114, 2}},   {"geffen.gat", {120, 70, 2}},
        {"morocc.gat", {156, 93, 2}},    {"aldebaran.gat", {140, 120, 0}},
        {"new_1-1.gat", {49, 113, 0}},
    };

    if (auto it = k.find(map); it != k.end()) return it->second;

    return {189, 132, 1};
}

}  // namespace arkan::poseidon::interface::ro::model
