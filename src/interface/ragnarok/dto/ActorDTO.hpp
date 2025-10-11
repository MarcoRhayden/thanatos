#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace arkan::thanatos::interface::ro::dto
{

using AccountID = std::array<uint8_t, 4>;  // Fixed 4-byte little-endian RID used on wire.
// ワイヤ上で使う固定 4 バイトのアカウントID（LE）。クライアント/サーバ間でそのまま送る。

// -----------------------------------------------------------------------------
// SyncInfo → ServerPacket::SyncAccount (0x0283)
// - Minimal handshake telling the client which actor/account we are.
// - Only carries the raw AccountID; packet body is exactly those 4 bytes.
// クライアントへ「自分はこのアカウントだ」と通知する最小ハンドシェイク。
// 4 バイトの AccountID のみを送るシンプルな DTO。
// -----------------------------------------------------------------------------
struct SyncInfo
{
    AccountID account_id;
};

// -----------------------------------------------------------------------------
// LookToInfo → ServerPacket::LookTo (0x009C)
// - Requests the client to face a direction. On classic clients this controls
//   head (and sometimes body) orientation.
// - `head_direction` typically 0..7 (NESW diagonals). Default = 4 (south).
// クライアントの向きを変更。古いクライアントでは頭（場合により体）向きも制御。
// `head_direction` は通常 0..7（方角）。デフォルトは 4（南向き）。
// -----------------------------------------------------------------------------
struct LookToInfo
{
    AccountID account_id;
    uint8_t head_direction = 4;
};

// -----------------------------------------------------------------------------
// ActorDisplayInfo → ServerPacket::ActorInfoSelf (0x0A30)
// - High-level label set shown on nameplates: character, party, guild, title.
// - All fields are serialized as fixed-size, NUL-padded strings on the wire;
//   the mapper handles clamping/padding to protocol limits (24 bytes each).
// 名前表示（キャラ名/パーティ/ギルド/称号）のまとめ DTO。
// 送信時は固定長 NUL パディング文字列。24 バイトへの切詰め/埋めはマッパ側で処理。
// -----------------------------------------------------------------------------
struct ActorDisplayInfo
{
    AccountID account_id;
    std::string name;
    std::string party_name;
    std::string guild_name;
    std::string title;
};

// -----------------------------------------------------------------------------
// ActorNameInfo → ServerPacket::ActorNameSelf (0x0095)
// - Short form used to push/refresh only the character name label.
// - Kept separate from ActorDisplayInfo to avoid over-sending when only
//   the main name changes.
// キャラ名だけを更新する軽量版。不要なフィールド送信を避けるために分離。
// -----------------------------------------------------------------------------
struct ActorNameInfo
{
    AccountID account_id;
    std::string name;
};

}  // namespace arkan::thanatos::interface::ro::dto
