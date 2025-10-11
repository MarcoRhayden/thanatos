#pragma once

#include "interface/ragnarok/dto/ActorDTO.hpp"
#include "interface/ragnarok/protocol/Codec.hpp"
#include "interface/ragnarok/protocol/Opcodes.hpp"

namespace arkan::thanatos::interface::ro::mappers
{

namespace proto = arkan::thanatos::interface::ro::protocol;
namespace dto = arkan::thanatos::interface::ro::dto;
using proto::Packet;

/* -----------------------------------------------------------------------------
   0x0095 ActorNameSelf
   - Sends the player's own name paired with account id.
   - 自キャラ名をアカウントIDと一緒に送信する。
   - Client expects exactly 24 bytes (NUL padded) for the name field.
   - 名前は厳密に24バイト（NULパディング）である必要がある。
----------------------------------------------------------------------------- */
inline Packet to_packet(const dto::ActorNameInfo& info)
{
    Packet p;
    proto::put16(p, static_cast<uint16_t>(proto::ServerPacket::ActorNameSelf));
    proto::putFixed(p, info.account_id.data(), info.account_id.size());  // aID (4)
    proto::putZ(p, info.name, 24);                                       // name[24]
    return p;
}

/* -----------------------------------------------------------------------------
   0x0283 SyncAccount
   - Minimal "I`m here" sync using the account id.
   - アカウントIDで存在同期を行う最小パケット。
   - Often used right after map login handshakes.
   - マップログイン直後のハンドシェイクでよく使われる。
----------------------------------------------------------------------------- */
inline Packet to_packet(const dto::SyncInfo& info)
{
    Packet p;
    proto::put16(p, static_cast<uint16_t>(proto::ServerPacket::SyncAccount));
    proto::putFixed(p, info.account_id.data(), info.account_id.size());  // aID (4)
    return p;
}

/* -----------------------------------------------------------------------------
   0x009C LookTo (self)
   - Updates facing direction (head/body); many clients only use head here.
   - 向き（頭/体）を更新。多くのクライアントは頭向きのみ参照。
   - Two middle bytes are kept as 0 for compatibility in this layout.
   - 互換のため中央2バイトは0のままにする。
----------------------------------------------------------------------------- */
inline Packet to_packet(const dto::LookToInfo& info)
{
    Packet p;
    proto::put16(p, static_cast<uint16_t>(proto::ServerPacket::LookTo));
    proto::putFixed(p, info.account_id.data(), info.account_id.size());  // aID (4)
    p.push_back(0);  // body dir (kept 0 in current layout) / 体向き（現行は0固定）
    p.push_back(0);  // reserved / 予約
    p.push_back(info.head_direction);  // head dir / 頭向き
    return p;
}

/* -----------------------------------------------------------------------------
   0x0A30 ActorInfoSelf
   - Rich identity block: name/party/guild/title for the local actor.
   - 自キャラの拡張情報：名前/パーティ/ギルド/称号。
   - Final dword is a flags placeholder; keep 0 unless you wire feature bits.
   - 末尾のDWORDはフラグ占位。機能ビットを使わない限り0のまま。
----------------------------------------------------------------------------- */
inline Packet to_packet(const dto::ActorDisplayInfo& info)
{
    Packet p;
    proto::put16(p, static_cast<uint16_t>(proto::ServerPacket::ActorInfoSelf));
    proto::putFixed(p, info.account_id.data(), info.account_id.size());  // aID (4)
    proto::putZ(p, info.name, 24);                                       // name[24]     / 名前
    proto::putZ(p, info.party_name, 24);  // party[24]    / パーティ名
    proto::putZ(p, info.guild_name, 24);  // guild[24]    / ギルド名
    proto::putZ(p, info.title, 24);       // title[24]    / 称号
    proto::put32(p, 0);                   // flags (placeholder) / フラグ（占位）
    return p;
}

}  // namespace arkan::thanatos::interface::ro::mappers
