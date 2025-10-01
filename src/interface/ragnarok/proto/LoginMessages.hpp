#pragma once

#include <array>
#include <string>
#include <string_view>
#include <vector>

#include "Codec.hpp"

namespace arkan::thanatos::interface::ro::proto::login
{
using arkan::thanatos::interface::ro::proto::Packet;
using namespace arkan::thanatos::interface::ro::proto;

/* ========= Handshakes ========= */

// 01DC: secure_login_key (responds to 01DB/0204)
inline Packet SecureLoginKey01DC()
{
    Packet p;
    put16(p, 0x01DC);
    put16(p, 0x0014);               // len=20
    p.insert(p.end(), 0x10, 0x00);  // 16 zeros
    return p;
}

// 0AE3: login_token (responds to 0ACF/0C26)
inline Packet LoginToken0AE3(std::string_view token = "OpenkoreClientToken")
{
    Packet p;
    put16(p, 0x0AE3);
    put16(p, 0x002F);
    put32(p, 0u);          // flags
    putZ(p, "S1000", 20);  // build/plat tag accepted by many clients
    // Z-string (terminated in 0)
    p.insert(p.end(), token.begin(), token.end());
    p.push_back(0x00);
    return p;
}

/* ========= Account server list ========= */

// 0069: account_server_info (list of CHARs) – standard LE format
inline Packet AccountServer0069(const std::array<uint8_t, 4>& sessionID,
                                const std::array<uint8_t, 4>& accountID,
                                const std::array<uint8_t, 4>& sessionID2,
                                const std::array<uint8_t, 4>& hostIp, uint16_t hostPortLE,
                                std::string_view serverName20, uint32_t usersOnline, bool male)
{
    Packet p;
    put16(p, 0x0069);
    const size_t len_pos = p.size();
    put16(p, 0x0000);  // patch later

    putFixed(p, sessionID.data(), sessionID.size());
    putFixed(p, accountID.data(), accountID.size());
    putFixed(p, sessionID2.data(), sessionID2.size());

    p.insert(p.end(), 30, 0x00);
    p.push_back(male ? 1 : 0);

    putFixed(p, hostIp.data(), hostIp.size());
    put16(p, hostPortLE);

    std::string name(serverName20);
    if (name.size() > 20) name.resize(20);
    putZ(p, name, 20);

    put32(p, usersOnline ? usersOnline : 100);
    p.push_back(0x00);
    p.push_back(0x00);

    const uint16_t total_len = static_cast<uint16_t>(p.size());
    p[len_pos + 0] = uint8_t(total_len & 0xFF);
    p[len_pos + 1] = uint8_t((total_len >> 8) & 0xFF);
    return p;
}

// 0AC4: account_server_info (long variant; some modern clients ask for it)
inline Packet AccountServer0AC4(const std::array<uint8_t, 4>& sessionID,
                                const std::array<uint8_t, 4>& accountID,
                                const std::array<uint8_t, 4>& sessionID2,
                                const std::array<uint8_t, 4>& hostIp, uint16_t hostPortLE,
                                std::string_view serverName20, uint32_t usersOnline, bool male)
{
    Packet p;
    put16(p, 0x0AC4);
    put16(p, 0x00E0);

    putFixed(p, sessionID.data(), sessionID.size());
    putFixed(p, accountID.data(), accountID.size());
    putFixed(p, sessionID2.data(), sessionID2.size());

    p.insert(p.end(), 4, 0x00);   // lastloginip
    p.insert(p.end(), 26, 0x00);  // lastLoginTime
    p.push_back(male ? 1 : 0);
    p.insert(p.end(), 17, 0x00);

    putFixed(p, hostIp.data(), hostIp.size());
    put16(p, hostPortLE);

    std::string name(serverName20);
    if (name.size() > 20) name.resize(20);
    putZ(p, name, 20);

    put32(p, usersOnline ? usersOnline : 100);

    if (p.size() < 0x00E0) p.insert(p.end(), 0x00E0 - p.size(), 0x00);
    return p;
}

}  // namespace arkan::thanatos::interface::ro::proto::login
