#pragma once

#include <cstdint>
#include <string>

namespace arkan::thanatos::interface::ro::dto
{

// -----------------------------------------------------------------------------
// WarpInfo → ServerPacket::Warp (0x0091)
// Teleports/forces the client to load a map and place the actor at x,y.
// Mapper will NUL-pad the map name to 16 bytes; do not include file extensions.
// クライアントにマップ読み込みと座標移動を指示するパケット。
// マップ名はマッパー側で16バイトNULパディング。拡張子は不要。
// -----------------------------------------------------------------------------
struct WarpInfo
{
    std::string map_name;  // e.g. "prontera" (no ".gat/.rsw")
                           // 例: "prontera"（拡張子不要）
    uint16_t x;            // target X (client grid units) / 目標X（グリッド座標）
    uint16_t y;            // target Y (client grid units) / 目標Y（グリッド座標）
};

// -----------------------------------------------------------------------------
// MapLoadedInfo → ServerPacket::MapLoaded (0x02EB)
// Signals “map is ready” along with packed A3 coords and visible area size.
// tick is a monotonic millisecond counter used by some clients for timing.
// マップ準備完了を通知。A3座標と可視領域サイズを併せて送る。
// tick は単調増加のミリ秒カウンタで、クライアントのタイミングに利用される。
// -----------------------------------------------------------------------------
struct MapLoadedInfo
{
    uint32_t tick;        // monotonic ms (use protocol::tick_ms()) / 単調ミリ秒
    uint16_t x;           // spawn X / 出現X
    uint16_t y;           // spawn Y / 出現Y
    uint8_t dir;          // facing 0..15 / 向き 0..15
    uint8_t x_size = 14;  // mini-map visible width (tiles) / 可視幅
    uint8_t y_size = 14;  // mini-map visible height (tiles) / 可視高さ
};

// -----------------------------------------------------------------------------
// AcceptEnterInfo → ServerPacket::AcceptEnter (0x0073)
// Final acknowledgement for entering the map server, reiterating position/dir.
// マップ入場確定の応答。座標と向きを再提示する。
// -----------------------------------------------------------------------------
struct AcceptEnterInfo
{
    uint16_t x;   // confirm X / 確定X
    uint16_t y;   // confirm Y / 確定Y
    uint8_t dir;  // confirm facing / 向きの確定
};

// -----------------------------------------------------------------------------
// LoadConfirm → ServerPacket::LoadConfirm (0x0B1B)
// Simple “I’m ready” signal with no payload. Empty struct acts as a marker.
// ペイロード無しの準備完了シグナル。空構造体をマーカーとして使用。
// -----------------------------------------------------------------------------
struct LoadConfirm
{
};

}  // namespace arkan::thanatos::interface::ro::dto
