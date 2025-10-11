#pragma once
#include <array>
#include <cstdint>
#include <string>

namespace arkan::thanatos::interface::ro::dto
{

using AccountID = std::array<uint8_t, 4>;  // 4-byte RID used verbatim on the wire (LE).
// ワイヤ上でそのまま送る4バイトのアカウントID（LE）。

using IPAddress = std::array<uint8_t, 4>;  // IPv4 octets as-is (no text parsing here).
// IPv4 の 4 オクテット配列。文字列へ変換せず生のまま扱う。

// -----------------------------------------------------------------------------
// PreambleAccountID → special “raw bytes only” preamble (no opcode).
// Some clients expect the account id to precede the first opcode packet.
// 一部クライアントは最初のオペコード前にアカウントIDの生バイトを期待する。
// -----------------------------------------------------------------------------
struct PreambleAccountID
{
    AccountID account_id;
};

// -----------------------------------------------------------------------------
// PreambleCommand082D → fixed-format capability preamble (0x082D).
// No variable fields; the mapper writes a constant payload.
// 固定レイアウトのプレアンブル（0x082D）。可変フィールドなし。マッパ側で定数を書き込む。
// -----------------------------------------------------------------------------
struct PreambleCommand082D
{
};

// -----------------------------------------------------------------------------
// PreambleCommand09A0 → parametric preamble (0x09A0) with a 32-bit value.
// 32ビット値を1つ持つ可変プレアンブル（0x09A0）。
// -----------------------------------------------------------------------------
struct PreambleCommand09A0
{
    uint32_t value;
};

// -----------------------------------------------------------------------------
// CharListInfo → character slot entry (0x099D block-155).
// Encapsulates everything the selection screen needs to render a character.
// Defaults mirror a known-working legacy build; override only what you need.
// キャラ選択スロット（0x099D, 155バイト）のための全情報。
// 既存の安定挙動に合わせたデフォルトを設定。必要な項目だけ上書きすればよい。
// -----------------------------------------------------------------------------
struct CharListInfo
{
    // --- Core Identifiers / コア識別子 ---
    uint32_t char_id = 0;  // Database RID; some clients may display slot+1 instead.
                           // DB上のID。クライアントによってはスロット+1が使われる場合あり。
    uint8_t slot = 0;      // Slot index (0..N), used by some UIs for ordering.
                           // スロット番号（0..N）。UI の並び順に使用されることがある。

    // --- Basic Info / 基本情報 ---
    std::string name = "Thanatos";  // Will be NUL-padded to 24 bytes by the mapper.
                                    // 送信時に 24 バイトへ NUL パディング。
    bool is_male = true;
    uint16_t base_level = 99;  // Display-only in the char list block.
                               // キャラ一覧では表示用途が主。
    uint16_t job_level = 50;   // Same as above / 同上。
    uint16_t job_id = 4074;    // Sorcerer (example). Ensure client GRF supports this ID.
                               // ソーサラー例。クライアント側GRFで対応していることが前提。
    uint64_t base_exp = 1993;
    uint64_t job_exp = 1993;
    uint32_t zeny = 2000000000;  // List-only preview; real value syncs later.
                                 // 一覧画面の見た目用。実値は後続パケットで同期。

    // --- Stats / ステータス ---
    uint8_t str = 99, agi = 99, vit = 99, int_ = 99, dex = 99, luk = 99;

    // --- Vitals / 生命力 ---
    uint16_t hp = 9999;
    uint16_t max_hp = 9999;
    uint16_t sp = 9999;
    uint16_t max_sp = 9999;

    // --- Appearance / 外見 ---
    uint16_t weapon = 0;  // Preview sprite layer; not equipment logic.
                          // 見た目用スプライト層。装備ロジックではない。
    uint16_t shield = 0;
    uint16_t head_low = 0;
    uint16_t head_mid = 0;
    uint16_t hair_color = 6;  // Palette index; must exist in client palettes.
                              // パレット番号。クライアント側パレットに存在する必要あり。
    uint16_t clothes_color = 0;

    // --- Location / 位置 ---
    std::string map_name = "prontera";  // Will be NUL-padded to 16 bytes (no extension needed).
                                        // 16 バイトに NUL パディング（拡張子不要）。

    // --- Misc Flags / その他のフラグ ---
    uint16_t option = 0;            // Bitset for visual/status options.
                                    // 見た目/状態のビットフラグ。
    uint16_t stance = 0;            // Pose/stance hint; often 0.
                                    // 姿勢ヒント。多くは 0。
    uint16_t manner = 0;            // Karma-like UI value; often 0.
                                    // カルマ風のUI値。多くは 0。
    uint16_t stat_points = 0;       // Unspent status points (display).
                                    // 未使用ステータスポイント（表示用）。
    uint8_t unknown_flag_c99 = 99;  // Reserved byte kept for legacy compatibility.
                                    // 互換性のために残す予約バイト。
    uint16_t rename_flag_v99 = 99;  // Rename marker (client-dependent semantics).
                                    // 改名フラグ（クライアント実装依存）。
};

// -----------------------------------------------------------------------------
// AttackRangeInfo → ServerPacket::AttackRange (0x013A).
// Small HUD hint telling the client current attack range.
// クライアントへ攻撃射程を通知する小さなHUD更新。
// -----------------------------------------------------------------------------
struct AttackRangeInfo
{
    uint16_t range;
};

// -----------------------------------------------------------------------------
// HpSpUpdateInfo → ServerPacket::HpSpUpdate (0x00B0).
// Fast path to update HP/SP bars without a full stats refresh.
// HP/SP だけを素早く更新する軽量パケット。フルステ更新不要時に有効。
// -----------------------------------------------------------------------------
struct HpSpUpdateInfo
{
    uint16_t hp;
    uint16_t max_hp;
    uint16_t sp;
    uint16_t max_sp;
};

// -----------------------------------------------------------------------------
// CharacterStatsInfo → ServerPacket::StatsInfo (0x00BD).
// Mirrors the legacy layout so mappers can serialize 1:1. Defaults mirror the
// old inlined function; adjust per session before emitting.
// レガシーの並び順に合わせたDTO。デフォルトは旧インライン関数に準拠。
// 送出前にセッションごとに上書きする想定。
// -----------------------------------------------------------------------------
struct CharacterStatsInfo
{
    // Defaults derived from the original code path.
    // 旧コードから持ってきた初期値。
    uint16_t points_free = 100;
    std::array<uint8_t, 12> stats_base = {99, 99, 99, 99, 99, 99, 0, 0, 0, 0, 0, 0};
    uint16_t aspd1 = 50, aspd2 = 50;
    uint16_t hp = 12, max_hp = 9999;
    uint16_t sp = 9999;
    uint16_t zeny = 0;
    uint16_t weight = 0;
    uint16_t weight_max = 5000;
    uint16_t walk_speed = 1;
    uint16_t job_level = 3;
    uint16_t base_level = 1;
    uint16_t status_points = 0;
    uint16_t attack_min = 10;
    uint16_t attack_max = 10;
    uint16_t magic_attack_min = 5;
    uint16_t magic_attack_max = 5;
    uint16_t defense = 100;
    uint16_t magic_defense = 100;
    uint16_t hit = 1;
    uint16_t flee = 150;
    uint16_t critical = 0;
    uint16_t karma = 0;
};

// -----------------------------------------------------------------------------
// RedirectInfo → map connect redirects (0x0071 short / 0x0AC5 full).
// Short: GID + map + IP/port.
// Full:  adds account/session cookies and sex. Set `use_full_redirect=true` to
//        emit 0x0AC5; otherwise 0x0071 is used.
// マップ接続のリダイレクト。短縮版は GID/マップ/IP/ポートのみ。
// 完全版はアカウント/セッション情報と性別を含む。`use_full_redirect=true` で 0x0AC5。
// -----------------------------------------------------------------------------
struct RedirectInfo
{
    AccountID char_id;     // GID (actor id on map server).
                           // マップ側で使う GID。
    std::string map_name;  // 16 bytes NUL-padded by mapper.
                           // マッパ側で 16 バイト NUL パディング。
    IPAddress ip;          // Target map server IPv4.
                           // 送信先マップサーバの IPv4。
    uint16_t port;         // Mapper writes in LE as required by protocol.
                           // プロトコル仕様で LE に変換して書き込む。

    // Fields used only by the full redirect (0x0AC5).
    // 完全版（0x0AC5）のみで使用。
    AccountID account_id{};
    AccountID session_a{};
    AccountID session_b{};
    uint8_t sex = 0;  // 1=male, 0=female (client expects this exact bit).
                      // 1=男, 0=女。クライアントはこのビットを期待。
    bool use_full_redirect = false;
};

}  // namespace arkan::thanatos::interface::ro::dto
