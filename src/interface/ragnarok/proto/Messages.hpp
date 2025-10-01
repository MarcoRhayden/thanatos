#pragma once

#include <array>
#include <string>
#include <string_view>

#include "Codec.hpp"

namespace arkan::thanatos::interface::ro::proto::msg
{
using arkan::thanatos::interface::ro::proto::Packet;
using namespace arkan::thanatos::interface::ro::proto;

/* ====================== MAP CORE ====================== */

// 0283: sync/account
inline Packet SyncAccount(const std::array<uint8_t, 4>& accountID)
{
    Packet p;
    put16(p, 0x0283);
    putFixed(p, accountID.data(), accountID.size());
    return p;
}

// 02EB: map_loaded (tick + A3 + sizes + zeros)
inline Packet MapLoaded02EB(uint32_t tick, uint16_t x, uint16_t y, uint8_t dir, uint8_t xSize = 14,
                            uint8_t ySize = 14)
{
    Packet p;
    put16(p, 0x02EB);
    put32(p, tick);
    putCoordsA3(p, x, y, dir);
    p.push_back(xSize);
    p.push_back(ySize);
    put16(p, 0);
    return p;
}

// 0073: accept_enter
inline Packet AcceptEnter0073(uint16_t x, uint16_t y, uint8_t dir)
{
    Packet p;
    put16(p, 0x0073);
    put16(p, x);
    put16(p, y);
    p.push_back(dir);
    put16(p, 0);
    put16(p, 0);
    return p;
}

// 0091: warp/teleport to map (force position on client)
inline Packet Warp0091(std::string_view map16, std::uint16_t x, std::uint16_t y)
{
    Packet p;
    put16(p, 0x0091);

    // clamp to maximum 16 bytes
    const std::string_view name = (map16.size() <= 16) ? map16 : map16.substr(0, 16);
    putZ(p, name, 16);  // 16 bytes, Z-string (fill with 0)

    put16(p, x);
    put16(p, y);
    return p;
}

/* ====================== HUD / STATE ====================== */

// 013A: attack range
inline Packet AttackRange013A(uint16_t range)
{
    Packet p;
    put16(p, 0x013A);
    put16(p, range);
    return p;
}

// 00BD: stats_info
inline Packet Stats00BD()
{
    Packet p;
    put16(p, 0x00BD);
    auto putv = [&](uint16_t v) { put16(p, v); };
    putv(100);                                                // points_free
    p.insert(p.end(), {1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0});  // base/job?? flags/bytes
    putv(50);
    putv(50);  // aspd/??
    putv(12);
    putv(12);   // hp/hpmax
    putv(150);  // sp?
    putv(0);
    putv(0);
    putv(0);  // z/weight?
    putv(1);
    putv(1);  // walk,job,hair,weapon,base,job
    putv(0);
    putv(5000);
    putv(0);  // weight,max,statpoints
    putv(10);
    putv(10);
    putv(5);
    putv(5);
    putv(100);
    putv(100);
    putv(1);
    putv(150);
    putv(0);
    putv(0);
    return p;
}

// 00B0: hp/sp
inline Packet HpSp00B0(uint16_t hp, uint16_t hpMax, uint16_t sp, uint16_t spMax)
{
    Packet p;
    put16(p, 0x00B0);
    put16(p, hp);
    put16(p, hpMax);
    put16(p, sp);
    put16(p, spMax);
    return p;
}

// 0B1B: load confirm
inline Packet LoadConfirm0B1B()
{
    Packet p;
    put16(p, 0x0B1B);
    return p;
}

// 009C: look_to (self)
inline Packet LookTo009C(const std::array<uint8_t, 4>& accountID, uint8_t to = 4)
{
    Packet p;
    put16(p, 0x009C);
    putFixed(p, accountID.data(), accountID.size());
    p.push_back(0);
    p.push_back(0);
    p.push_back(to);
    return p;
}

// 009A: system chat
inline Packet SystemChat009A(std::string_view msg)
{
    Packet p;
    put16(p, 0x009A);
    const uint16_t len = uint16_t(2 + msg.size() + 1);  // size + msg + NUL
    put16(p, len);
    p.insert(p.end(), msg.begin(), msg.end());
    p.push_back(0);
    return p;
}

// 0A30: actor_info (self)
inline Packet ActorInfoSelf0A30(const std::array<uint8_t, 4>& accountID, std::string_view name24)
{
    Packet p;
    put16(p, 0x0A30);
    putFixed(p, accountID.data(), accountID.size());
    putZ(p, name24, 24);
    putZ(p, "", 24);
    putZ(p, "", 24);
    putZ(p, "", 24);
    put32(p, 0);
    return p;
}

// 0095: actor_name (self)
inline Packet ActorNameSelf0095(const std::array<uint8_t, 4>& accountID, std::string_view name24)
{
    Packet p;
    put16(p, 0x0095);
    putFixed(p, accountID.data(), accountID.size());
    putZ(p, name24, 24);
    return p;
}

/* ====================== CHAR LIST / REDIRECT ====================== */

// (preamble) 082D + 09A0 + raw accountID
inline Packet RawAccountIdPreamble(const std::array<uint8_t, 4>& accountID)
{
    Packet p;
    putFixed(p, accountID.data(), accountID.size());
    return p;  // no opcode at all
}
inline Packet Preamble082D()
{
    Packet p;
    put16(p, 0x082D);
    put16(p, 0x001D);
    p.push_back(0x02);
    p.push_back(0x00);
    p.push_back(0x00);
    p.push_back(0x02);
    p.push_back(0x02);
    p.insert(p.end(), 0x14, 0x00);
    return p;
}
inline Packet Preamble09A0(uint32_t v = 1)
{
    Packet p;
    put16(p, 0x09A0);
    put32(p, v);
    return p;
}

// 099D: character list (155 simplified layout)
inline Packet CharList099D_Block155(uint32_t cidLE, std::string_view name24, std::string_view map16,
                                    bool male)
{
    const uint16_t block = 155;
    const uint16_t len = uint16_t(4 + block);
    Packet p;
    put16(p, 0x099D);
    put16(p, len);

    std::vector<uint8_t> b(block, 0);
    std::memcpy(&b[0], &cidLE, 4);
    // name in [88..111]
    for (size_t i = 0; i < 24 && i < name24.size(); ++i) b[88 + i] = uint8_t(name24[i]);
    // map in [122..137]
    for (size_t i = 0; i < 16 && i < map16.size(); ++i) b[122 + i] = uint8_t(map16[i]);
    b[154] = male ? 1 : 0;

    p.insert(p.end(), b.begin(), b.end());
    return p;
}

// 0071: short redirect (GID, map, ip, port)
inline Packet Redirect0071(const std::array<uint8_t, 4>& gid, std::string_view map16,
                           const std::array<uint8_t, 4>& ip, uint16_t portLE)
{
    Packet p;
    put16(p, 0x0071);
    putFixed(p, gid.data(), gid.size());
    putZ(p, map16, 16);
    putFixed(p, ip.data(), ip.size());
    put16(p, portLE);
    return p;
}

// 0AC5: redirect full (cookies + ip/port/map)
inline Packet Redirect0AC5(const std::array<uint8_t, 4>& accountID,
                           const std::array<uint8_t, 4>& gid, const std::array<uint8_t, 4>& loginA,
                           const std::array<uint8_t, 4>& loginB, uint8_t sex,
                           const std::array<uint8_t, 4>& ip, uint16_t portLE,
                           std::string_view map16)
{
    Packet p;
    put16(p, 0x0AC5);
    putFixed(p, accountID.data(), accountID.size());
    putFixed(p, gid.data(), gid.size());
    putFixed(p, loginA.data(), loginA.size());
    putFixed(p, loginB.data(), loginB.size());
    p.push_back(sex);
    putFixed(p, ip.data(), ip.size());
    put16(p, portLE);
    putZ(p, map16, 16);
    return p;
}

}  // namespace arkan::thanatos::interface::ro::proto::msg
