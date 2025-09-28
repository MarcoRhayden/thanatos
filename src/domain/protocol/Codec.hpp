#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "domain/protocol/Packet.hpp"
#include "domain/protocol/PacketIds.hpp"
#include "domain/protocol/Packets.hpp"

namespace arkan::poseidon::domain::protocol
{

// ==== helpers LE ====
inline void write_u16(std::vector<std::uint8_t>& out, std::uint16_t v)
{
    out.push_back(static_cast<std::uint8_t>(v & 0xFF));
    out.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
}
inline void write_str(std::vector<std::uint8_t>& out, const std::string& s)
{
    write_u16(out, static_cast<std::uint16_t>(s.size()));
    out.insert(out.end(), s.begin(), s.end());
}

// ==== Default Packet Encode -> bytes ====
inline std::vector<std::uint8_t> Encode(const Packet& p)
{
    const std::uint16_t size = static_cast<std::uint16_t>(4u + p.payload.size());
    std::vector<std::uint8_t> out;
    out.reserve(size);
    write_u16(out, p.opcode);
    write_u16(out, size);
    out.insert(out.end(), p.payload.begin(), p.payload.end());
    return out;
}

// ==== builders used by services (dev) ====
inline Packet Make_LoginOk_ServerList(const std::vector<ServerEntry>& servers)
{
    Packet p;
    p.opcode = ids::S_LOGIN_OK_SERVER_LIST;
    write_u16(p.payload, static_cast<std::uint16_t>(servers.size()));
    for (const auto& s : servers)
    {
        write_str(p.payload, s.name);
        write_str(p.payload, s.host);
        write_u16(p.payload, s.port);
    }
    return p;
}

inline Packet Make_CharList(const std::vector<Character>& chars)
{
    Packet p;
    p.opcode = ids::S_CHAR_LIST;
    write_u16(p.payload, static_cast<std::uint16_t>(chars.size()));
    for (const auto& c : chars)
    {
        write_str(p.payload, c.name);
        write_str(p.payload, c.map);
        write_u16(p.payload, c.x);
        write_u16(p.payload, c.y);
    }
    return p;
}

inline Packet Make_ConnectToMap(const std::string& host, std::uint16_t port, const std::string& map,
                                std::uint16_t x, std::uint16_t y)
{
    Packet p;
    p.opcode = ids::S_CONNECT_TO_MAP;
    write_str(p.payload, host);
    write_u16(p.payload, port);
    write_str(p.payload, map);
    write_u16(p.payload, x);
    write_u16(p.payload, y);
    return p;
}

}  // namespace arkan::poseidon::domain::protocol
