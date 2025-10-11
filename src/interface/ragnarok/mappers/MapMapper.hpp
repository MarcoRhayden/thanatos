#pragma once

#include "interface/ragnarok/dto/MapDTO.hpp"
#include "interface/ragnarok/protocol/Codec.hpp"
#include "interface/ragnarok/protocol/Opcodes.hpp"

namespace arkan::thanatos::interface::ro::mappers
{

namespace proto = arkan::thanatos::interface::ro::protocol;
namespace dto = arkan::thanatos::interface::ro::dto;
using proto::Packet;

/* -----------------------------------------------------------------------------
   0x0091 Warp
   - Forces client to load a map and place the actor at (x,y).
   - クライアントにマップ遷移と座標(x,y)での配置を指示する。
   - Map name must be a NUL-padded 16-byte field; longer names are truncated.
   - マップ名はNULパディング16バイト。長い場合は切り詰める。
----------------------------------------------------------------------------- */
inline Packet to_packet(const dto::WarpInfo& info)
{
    Packet p;
    proto::put16(p, static_cast<uint16_t>(proto::ServerPacket::Warp));
    // enforce 16-byte, NUL-padded map name
    // 16バイト固定長のマップ名（NULパディング）
    const std::string_view name = (info.map_name.size() <= 16)
                                      ? info.map_name
                                      : std::string_view(info.map_name).substr(0, 16);
    proto::putZ(p, name, 16);
    proto::put16(p, info.x);  // X coordinate / X座標
    proto::put16(p, info.y);  // Y coordinate / Y座標
    return p;
}

/* -----------------------------------------------------------------------------
   0x02EB MapLoaded
   - Confirms client finished loading and sets initial A3 coordinates + view sizes.
   - クライアントのロード完了を通知し、A3座標＋視界サイズを設定する。
   - A3 packs x(10) | y(10) | dir(4) into 3 bytes little-endian.
   - A3はx(10) | y(10) | dir(4) をLE 3バイトにパックする。
----------------------------------------------------------------------------- */
inline Packet to_packet(const dto::MapLoadedInfo& info)
{
    Packet p;
    proto::put16(p, static_cast<uint16_t>(proto::ServerPacket::MapLoaded));
    proto::put32(p, info.tick);                       // server tick / サーバチック
    proto::putCoordsA3(p, info.x, info.y, info.dir);  // A3 packed coords / A3座標
    p.push_back(info.x_size);                         // client view width / 画面幅
    p.push_back(info.y_size);                         // client view height / 画面高さ
    proto::put16(p, 0);                               // reserved / 予約
    return p;
}

/* -----------------------------------------------------------------------------
   0x0073 AcceptEnter
   - Finalizes spawn on the map at (x,y,dir) after login/warp.
   - ログイン/ワープ後の最終的なマップ配置を確定する。
   - Two trailing u16 are zero for this layout.
   - 末尾のu16×2は本レイアウトでは0のまま。
----------------------------------------------------------------------------- */
inline Packet to_packet(const dto::AcceptEnterInfo& info)
{
    Packet p;
    proto::put16(p, static_cast<uint16_t>(proto::ServerPacket::AcceptEnter));
    proto::put16(p, info.x);  // X / X座標
    proto::put16(p, info.y);  // Y / Y座標
    p.push_back(info.dir);    // facing dir / 向き
    proto::put16(p, 0);       // reserved / 予約
    proto::put16(p, 0);       // reserved / 予約
    return p;
}

/* -----------------------------------------------------------------------------
   0x0B1B LoadConfirm
   - Minimal “I’m ready” notice once the client is fully in the map.
   - クライアントがマップ内で準備完了になったことを知らせる簡易通知。
----------------------------------------------------------------------------- */
inline Packet to_packet(const dto::LoadConfirm& /*info*/)
{
    Packet p;
    proto::put16(p, static_cast<uint16_t>(proto::ServerPacket::LoadConfirm));
    return p;
}

}  // namespace arkan::thanatos::interface::ro::mappers
