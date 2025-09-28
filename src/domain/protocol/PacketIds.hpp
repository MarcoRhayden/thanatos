#pragma once

#include <cstdint>

namespace arkan::poseidon::domain::protocol::ids
{
constexpr std::uint16_t C_LOGIN_REQ = 0x0064;             // client -> login
constexpr std::uint16_t S_LOGIN_OK_SERVER_LIST = 0x1069;  // login -> client

constexpr std::uint16_t C_CHAR_LIST_REQ = 0x0065;  // cliente -> char
constexpr std::uint16_t S_CHAR_LIST = 0x106B;      // char -> cliente

constexpr std::uint16_t C_CHAR_SELECT = 0x0066;     // cliente -> char
constexpr std::uint16_t S_CONNECT_TO_MAP = 0x1071;  // char -> cliente (handoff RO)
}  // namespace arkan::poseidon::domain::protocol::ids
