#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace arkan::thanatos::interface::ro::dto
{

using AccountID = std::array<uint8_t, 4>;
using IPAddress = std::array<uint8_t, 4>;
using SessionID = std::array<uint8_t, 4>;

// Represents a secure login key packet (0x01DC). This packet has no variable data.
// セキュアログインキーパケット（0x01DC）を表します。このパケットには可変データはありません。
struct SecureLoginKeyInfo
{
};

// Represents a login token packet (0x0AE3).
// ログイントークンパケット（0x0AE3）を表します。
struct LoginTokenInfo
{
    std::string token = "ClientToken";
};

// Represents the data needed to describe a single character server to the client.
// 単一のキャラクターサーバーをクライアントに記述するために必要なデータを表します。
struct AccountServerInfo
{
    SessionID session_id;
    AccountID account_id;
    SessionID session_id2;
    std::array<uint8_t, 4> host_ip;
    uint16_t host_port;  // Mapper will handle little-endian conversion
    std::string server_name;
    uint32_t users_online;
    bool is_male;

    // Flag to determine if the modern packet format (0x0AC4) should be used.
    // モダンなパケット形式（0x0AC4）を使用するかどうかを決定するフラグ。
    bool use_modern_format = false;
};

}  // namespace arkan::thanatos::interface::ro::dto