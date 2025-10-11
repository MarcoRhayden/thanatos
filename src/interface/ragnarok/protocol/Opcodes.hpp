#pragma once

#include <cstdint>

namespace arkan::thanatos::interface::ro::protocol
{

// -----------------------------------------------------------------------------
// ServerPacket
// Canonical list of server->client opcodes used by our interface layer.
// Keep this as the single source of truth so mappers/flows remain consistent.
// サーバ→クライアントのオペコードを集約。インタフェース層の単一情報源として維持し、
// マッパー/フロー間の不整合を防ぐ。
// -----------------------------------------------------------------------------
enum class ServerPacket : uint16_t
{
    // ------------------------- Map Core / マップ基本 -------------------------
    // Sent after map connection to bind account context on the map server.
    // マップ接続後にアカウント文脈をバインド。
    SyncAccount = 0x0283,

    // "Map loaded" notification: tick + packed A3 coords + map tile sizes.
    // マップ読み込み完了通知：tick + A3座標 + マップタイルサイズ。
    MapLoaded = 0x02EB,

    // Accepts character’s entry and confirms spawn position.
    // キャラの入場承認と出現位置の確定。
    AcceptEnter = 0x0073,

    // Teleport/warp to map with forced coordinates.
    // 強制座標付きワープ/テレポート。
    Warp = 0x0091,

    // ---------------------- HUD / State / HUD・状態 -----------------------
    // Sets/updates attack range shown on the client.
    // クライアント表示の攻撃射程を設定/更新。
    AttackRange = 0x013A,

    // Bulk stat block (hp/sp/aspd/etc.). Version varies per client; keep layout stable.
    // ステータス一括送信（HP/SP/ASPD等）。クライアント差異あり、レイアウトの安定性に注意。
    StatsInfo = 0x00BD,

    // HP/SP incremental refresh.
    // HP/SPの増分更新。
    HpSpUpdate = 0x00B0,

    // Signals that the initial load sequence completed.
    // 初期ロード完了の合図。
    LoadConfirm = 0x0B1B,

    // Sets head/body facing direction (self).
    // 自身の向き（頭/体）を設定。
    LookTo = 0x009C,

    // System chat line (server message).
    // システムチャット行（サーバメッセージ）。
    SystemChat = 0x009A,

    // Self actor info (names, titles). Used early after spawn.
    // 自身のアクター情報（名称/称号）。スポーン直後に使用。
    ActorInfoSelf = 0x0A30,

    // Self name broadcast (short form).
    // 自身の名称通知（短形式）。
    ActorNameSelf = 0x0095,

    // --------------- Char List / Redirect / キャラ一覧・遷移 ---------------
    // Fixed preamble some clients expect before char list traffic.
    // 一部クライアントがキャラ一覧前に要求する前置き。
    Preamble082D = 0x082D,

    // Additional preamble knob (often “version”/flags).
    // 追加の前置き（バージョン/フラグ用途）。
    Preamble09A0 = 0x09A0,

    // Character list entry (155-byte block flavor in our impl).
    // キャラ一覧エントリ（本実装では155バイト版）。
    CharListBlock = 0x099D,

    // Short redirect: gid + map + ip + port.
    // 簡略リダイレクト：GID + マップ + IP + ポート。
    RedirectShort = 0x0071,

    // Full redirect: includes session cookies and sex bit.
    // 完全リダイレクト：セッション情報や性別ビットを含む。
    RedirectFull = 0x0AC5,

    // ======================= Login Flow / ログイン系 =======================
    // Secure login key (responds to 0x01DB/0x0204).
    // セキュアキー応答（0x01DB/0x0204に対する返答）。
    SecureLoginKey = 0x01DC,

    // Login token blob (responds to 0x0ACF/0x0C26).
    // ログイントークン（0x0ACF/0x0C26への返答）。
    LoginToken = 0x0AE3,

    // Classic server list entry (0069). Many clients still rely on this.
    // 旧来のサーバ一覧（0069）。依存クライアントが多い。
    AccountServerInfo = 0x0069,

    // Modern/extended server list (0AC4). Some new clients require it.
    // 拡張サーバ一覧（0AC4）。新しめのクライアントで必須な場合あり。
    AccountServerInfoV2 = 0x0AC4,
};

}  // namespace arkan::thanatos::interface::ro::protocol
