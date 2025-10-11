#pragma once

#include "interface/ragnarok/dto/LoginDTO.hpp"
#include "interface/ragnarok/protocol/Codec.hpp"
#include "interface/ragnarok/protocol/Opcodes.hpp"

namespace arkan::thanatos::interface::ro::mappers
{

namespace proto = arkan::thanatos::interface::ro::protocol;
namespace dto = arkan::thanatos::interface::ro::dto;
using proto::Packet;

/* -------------------------------------------------------------------------
   SecureLoginKey (0x01DC)
   - Fixed-size packet used in the secure handshake phase (reply to 0x01DB/0x0204).
   - Exactly 20 bytes total: [opcode:2][len:2=0x0014][16 zero bytes]
   - セキュアハンドシェイク段階で使用する固定長パケット（0x01DB/0x0204 への応答）。
   - 合計 20 バイト固定: [opcode:2][len:2=0x0014][ゼロ16バイト]
------------------------------------------------------------------------- */
inline Packet to_packet(const dto::SecureLoginKeyInfo& /*info*/)
{
    Packet p;
    proto::put16(p, static_cast<uint16_t>(proto::ServerPacket::SecureLoginKey));  // 0x01DC
    proto::put16(p, 0x0014);        // total length = 20 bytes
    p.insert(p.end(), 0x10, 0x00);  // 16 zero bytes
    return p;
}

/* -------------------------------------------------------------------------
   LoginToken (0x0AE3)
   - Token carrier (reply to 0x0ACF/0x0C26). Length MUST match actual payload.
   - Layout:
       opcode(2) + len(2) +
       flags(4, zero) +
       tag Z(20) = "S1000" padded with NUL +
       token bytes + NUL
   - ログイントークン（0x0ACF/0x0C26 への応答）。len は実データ長と一致必須。
   - 構造:
       opcode(2) + len(2) +
       flags(4=0) +
       タグ Z(20) "S1000" + パディング +
       token 本体 + NUL
------------------------------------------------------------------------- */
inline Packet to_packet(const dto::LoginTokenInfo& info)
{
    Packet p;
    proto::put16(p, static_cast<uint16_t>(proto::ServerPacket::LoginToken));  // 0x0AE3

    // Compute precise total length:
    // opcode(2) + len(2) + flags(4) + tag(20) + token + NUL(1)
    const uint16_t len = static_cast<uint16_t>(4 + 4 + 20 + info.token.size() + 1);
    proto::put16(p, len);

    proto::put32(p, 0u);          // flags = 0
    proto::putZ(p, "S1000", 20);  // client-accepted build/plat tag (padded to 20)
    p.insert(p.end(), info.token.begin(), info.token.end());
    p.push_back(0x00);  // NUL terminator for token
    return p;
}

/* -------------------------------------------------------------------------
   AccountServerInfo (0x0069 / 0x0AC4)
   - Server list entry. There are two variants:
     * Classic (0x0069): variable length with a patched len field.
     * Modern  (0x0AC4): fixed 0x00E0 bytes.
   - The client is VERY sensitive to offsets. Keep padding counts EXACT.
   - アカウントサーバ一覧。2 種類あり:
     * 旧式 (0x0069): 可変長。末尾で len をパッチする。
     * 新式 (0x0AC4): 固定長 0x00E0。
   - クライアントはオフセットに非常に敏感。パディング数は厳密に一致させること。
------------------------------------------------------------------------- */
inline Packet to_packet(const dto::AccountServerInfo& info)
{
    Packet p;

    if (info.use_modern_format)
    {
        // --------------------------- 0x0AC4 (modern, fixed-size) ---------------------------
        proto::put16(p, static_cast<uint16_t>(proto::ServerPacket::AccountServerInfoV2));  // 0x0AC4
        proto::put16(p, 0x00E0);  // fixed total length

        // Session & identity triplet
        proto::putFixed(p, info.session_id.data(), info.session_id.size());
        proto::putFixed(p, info.account_id.data(), info.account_id.size());
        proto::putFixed(p, info.session_id2.data(), info.session_id2.size());

        // Exact padding/fields per legacy (must not change counts)
        // レガシー通りのパディング（数を変えない）
        p.insert(p.end(), 4, 0x00);         // lastloginip (4 zeros)
        p.insert(p.end(), 26, 0x00);        // lastLoginTime (26 zeros)
        p.push_back(info.is_male ? 1 : 0);  // sex flag
        p.insert(p.end(), 17, 0x00);        // reserved (17 zeros)

        // Address & port (port is written LE directly, do not byteswap beforehand)
        // アドレスとポート（ポートはそのまま LE で書き込む）
        proto::putFixed(p, info.host_ip.data(), info.host_ip.size());
        proto::put16(p, info.host_port);

        // Server name: Z(20) padded/truncated
        // サーバ名: Z(20) パディング/トランケート
        std::string name(info.server_name);
        if (name.size() > 20) name.resize(20);
        proto::putZ(p, name, 20);

        // Online user count; fallback to 100 if zero (legacy parity)
        // 同時接続数；0 の場合は 100 を入れる（レガシー準拠）
        proto::put32(p, info.users_online ? info.users_online : 100);

        // Pad to exactly 0x00E0 bytes
        // ちょうど 0x00E0 バイトまでゼロ埋め
        if (p.size() < 0x00E0) p.insert(p.end(), 0x00E0 - p.size(), 0x00);
    }
    else
    {
        // --------------------------- 0x0069 (classic, variable) ---------------------------
        proto::put16(p, static_cast<uint16_t>(proto::ServerPacket::AccountServerInfo));  // 0x0069

        // Reserve space for len and patch it at the end.
        // 長さを後でパッチするため、先に 2 バイト分のスペースを確保。
        const size_t len_pos = p.size();
        proto::put16(p, 0x0000);  // placeholder, will be overwritten

        // Session triplet
        proto::putFixed(p, info.session_id.data(), info.session_id.size());
        proto::putFixed(p, info.account_id.data(), info.account_id.size());
        proto::putFixed(p, info.session_id2.data(), info.session_id2.size());

        // Exact 30 zeros, then 1 byte sex flag
        // 正確に 30 バイトのゼロ、その後 1 バイトの性別フラグ
        p.insert(p.end(), 30, 0x00);
        p.push_back(info.is_male ? 1 : 0);

        // Address & port (LE, no pre-swap)
        // アドレスとポート（LE そのまま）
        proto::putFixed(p, info.host_ip.data(), info.host_ip.size());
        proto::put16(p, info.host_port);

        // Server name: Z(20), trunc/pad
        // サーバ名: Z(20) 切り詰め/パディング
        std::string name(info.server_name);
        if (name.size() > 20) name.resize(20);
        proto::putZ(p, name, 20);

        // Online users (fallback 100), and final 2 zero bytes
        // 同時接続数（0 なら 100）、末尾に 2 バイトのゼロ
        proto::put32(p, info.users_online ? info.users_online : 100);
        p.push_back(0x00);
        p.push_back(0x00);

        // Patch the total length (little-endian) at len_pos
        // 末尾サイズ確定後、len_pos に LE で長さを書き戻す
        const uint16_t total_len = static_cast<uint16_t>(p.size());
        p[len_pos + 0] = static_cast<uint8_t>(total_len & 0xFF);
        p[len_pos + 1] = static_cast<uint8_t>((total_len >> 8) & 0xFF);
    }

    return p;
}

}  // namespace arkan::thanatos::interface::ro::mappers
