#pragma once

#include "interface/ragnarok/dto/CharacterDTO.hpp"
#include "interface/ragnarok/protocol/Codec.hpp"
#include "interface/ragnarok/protocol/Opcodes.hpp"

namespace arkan::thanatos::interface::ro::mappers
{

namespace proto = arkan::thanatos::interface::ro::protocol;
namespace dto = arkan::thanatos::interface::ro::dto;
using proto::Packet;

/* -----------------------------------------------------------------------------
   PreambleAccountID -> Packet (no-opcode payload)
   事前ヘッダ(アカウントID) -> パケット（オプコードなしの生データ）
   - Client expects raw account id bytes before subsequent preambles.
   - クライアントは次の前置パケットの前に生のアカウントIDを期待する。
----------------------------------------------------------------------------- */
inline Packet to_packet(const dto::PreambleAccountID& info)
{
    Packet p;
    // Special preamble: write only the payload without an opcode.
    // 特殊な前置: オプコードを持たず、ペイロードのみを書き込む。
    proto::putFixed(p, info.account_id.data(), info.account_id.size());
    return p;
}

/* -----------------------------------------------------------------------------
   0x082D (PreambleCommand082D)
   - Static shape used by many packetvers to prime the char list channel.
   - 多くのpacketverでキャラリスト用チャンネルを初期化する固定レイアウト。
----------------------------------------------------------------------------- */
inline Packet to_packet(const dto::PreambleCommand082D& /*info*/)
{
    Packet p;
    proto::put16(p, static_cast<uint16_t>(proto::ServerPacket::Preamble082D));
    proto::put16(p, 0x001D);
    // Header tail and padding bytes per client expectation
    // クライアント仕様に合わせたヘッダ末尾とパディング
    p.insert(p.end(), {0x02, 0x00, 0x00, 0x02, 0x02});
    p.insert(p.end(), 0x14, 0x00);
    return p;
}

/* -----------------------------------------------------------------------------
   0x09A0 (PreambleCommand09A0)
   - Single u32 value; often used as a version/flag toggle.
   - 単一のu32値。バージョン/フラグ切替に使われることが多い。
----------------------------------------------------------------------------- */
inline Packet to_packet(const dto::PreambleCommand09A0& info)
{
    Packet p;
    proto::put16(p, static_cast<uint16_t>(proto::ServerPacket::Preamble09A0));
    proto::put32(p, info.value);
    return p;
}

/* -----------------------------------------------------------------------------
   0x099D (CharListInfo)
   - Pure serializer: layout comes from client, values come from DTO.
   - 純粋なシリアライザ：レイアウトはクライアント仕様、値はDTOから供給。
----------------------------------------------------------------------------- */
inline Packet to_packet(const dto::CharListInfo& info)
{
    Packet p;
    proto::put16(p, 0x099D);

    std::vector<uint8_t> block;
    block.reserve(155);

    // Local writers bound to the block buffer
    // ブロックバッファにバインドしたローカル書き込み関数
    auto putV = [&](uint32_t v) { proto::wr32le(block, v); };
    auto putv = [&](uint16_t v) { proto::wr16le(block, v); };
    auto putC = [&](uint8_t v) { block.push_back(v); };
    auto putZ8 = [&](uint64_t v)
    {
        for (int i = 0; i < 8; ++i) block.push_back(static_cast<uint8_t>((v >> (8 * i)) & 0xFF));
    };
    auto putZstr = [&](std::string_view s, size_t n) { proto::putZ(block, s, n); };

    // ---- Sequential write (do not reorder) / 順次書き込み（順番厳守） ----

    // Client expects slot-index+1 in this field on some packetvers.
    // 一部packetverではここにスロット番号+1を期待する。
    putV(info.slot + 1);

    putZ8(info.base_exp);
    putV(info.zeny);
    putZ8(info.job_exp);
    putV(info.job_level);
    putV(0);  // opt2 (reserved / 実質予約)
    putV(info.option);
    putV(info.stance);
    putV(info.manner);
    putV(info.stat_points);

    putv(0);                                        // unknown/compat gap / 不明(互換ギャップ)
    putV(static_cast<uint16_t>(info.hp & 0xFFFF));  // hp (client expects u32 here on this layout)
                                                    // hp（このレイアウトではu32領域）
    putV(static_cast<uint16_t>(info.max_hp & 0xFFFF));  // max_hp
    putv(static_cast<uint16_t>(info.sp & 0xFFFF));      // sp (u16 field)
    putv(static_cast<uint16_t>(info.max_sp & 0xFFFF));  // max_sp
    putv(0);                                            // unknown/compat gap / 不明(互換ギャップ)
    putv(info.job_id);                                  // job/class id / ジョブID（クラスID）

    putV(info.weapon);
    putv(0);                // unknown/compat gap / 不明(互換ギャップ)
    putv(info.base_level);  // base level / ベースレベル
    putv(info.head_low);    // lower headgear / 下段装備
    putv(info.shield);      // shield / 盾
    putv(0);                // unknown/compat gap / 不明(互換ギャップ)
    putv(info.head_mid);    // mid headgear / 中段装備
    putv(0);  // redundant hair palette (keep 0 if layout requires) / 冗長（レイアウト準拠）
    putv(info.hair_color);     // hair color (palette index) / 髪色（パレット）
    putv(info.clothes_color);  // cloth dye or extra / 衣装染色（または拡張）

    putZstr(info.name, 24);  // char name / キャラ名
    putC(info.str);
    putC(info.agi);
    putC(info.vit);
    putC(info.int_);
    putC(info.dex);
    putC(info.luk);
    putC(info.slot);              // slot index / スロット番号
    putC(info.unknown_flag_c99);  // flag byte / フラグ
    putv(info.rename_flag_v99);   // rename flag / 改名フラグ
    putZstr(info.map_name, 16);   // start map / 開始マップ
    putV(0);                      // extra1 (delete date, etc.) / 予備（削除日など）
    putV(0);                      // extra2
    putV(0);                      // extra3
    putV(0);                      // extra4
    putC(info.is_male ? 1 : 0);   // sex (1=male,0=female) / 性別

    // Ensure fixed-size block (exactly 155 bytes)
    // 固定長ブロック（ちょうど155バイト）に揃える
    if (block.size() < 155)
        block.resize(155, 0);
    else if (block.size() > 155)
        block.resize(155);

    // Prepend the total length: opcode(2) + len(2) + body(155)
    // 総バイト長: オプコード(2) + 長さ(2) + 本文(155)
    const uint16_t packet_len = static_cast<uint16_t>(4 + 155);
    proto::put16(p, packet_len);
    p.insert(p.end(), block.begin(), block.end());

    return p;
}

/* -----------------------------------------------------------------------------
   Redirect (0071 / 0x0AC5)
   - Short vs Full redirect depends on `use_full_redirect`.
   - ショート/フルのどちらかは `use_full_redirect` で切替。
----------------------------------------------------------------------------- */
inline Packet to_packet(const dto::RedirectInfo& info)
{
    Packet p;
    if (info.use_full_redirect)
    {
        // 0x0AC5 Full redirect: account + gid + cookies + ip/port/map
        // 0x0AC5 フルリダイレクト: アカウント/GID/クッキー/IP/ポート/マップ
        proto::put16(p, static_cast<uint16_t>(proto::ServerPacket::RedirectFull));
        proto::putFixed(p, info.account_id.data(), info.account_id.size());
        proto::putFixed(p, info.char_id.data(), info.char_id.size());
        proto::putFixed(p, info.session_a.data(), info.session_a.size());
        proto::putFixed(p, info.session_b.data(), info.session_b.size());
        p.push_back(info.sex);
        proto::putFixed(p, info.ip.data(), info.ip.size());
        proto::put16(p, info.port);
        proto::putZ(p, info.map_name, 16);
    }
    else
    {
        // 0x0071 Short redirect: gid + map + ip/port
        // 0x0071 ショートリダイレクト: GID/マップ/IP/ポート
        proto::put16(p, static_cast<uint16_t>(proto::ServerPacket::RedirectShort));
        proto::putFixed(p, info.char_id.data(), info.char_id.size());
        proto::putZ(p, info.map_name, 16);
        proto::putFixed(p, info.ip.data(), info.ip.size());
        proto::put16(p, info.port);
    }
    return p;
}

/* -----------------------------------------------------------------------------
   0x013A (AttackRange)
   - Minimal two-field packet: opcode + u16 range.
   - 最小二項目パケット：オプコード + u16レンジ
----------------------------------------------------------------------------- */
inline Packet to_packet(const dto::AttackRangeInfo& info)
{
    Packet p;
    proto::put16(p, static_cast<uint16_t>(proto::ServerPacket::AttackRange));
    proto::put16(p, info.range);
    return p;
}

/* -----------------------------------------------------------------------------
   0x00B0 (HP/SP Update)
   - Four 16-bit values: hp, max_hp, sp, max_sp.
   - 4つの16ビット値：HP、MaxHP、SP、MaxSP。
----------------------------------------------------------------------------- */
inline Packet to_packet(const dto::HpSpUpdateInfo& info)
{
    Packet p;
    proto::put16(p, static_cast<uint16_t>(proto::ServerPacket::HpSpUpdate));
    proto::put16(p, info.hp);
    proto::put16(p, info.max_hp);
    proto::put16(p, info.sp);
    proto::put16(p, info.max_sp);
    return p;
}

/* -----------------------------------------------------------------------------
   0x00BD (StatsInfo)
   - Mirrors the legacy layout; all values supplied by DTO.
   - 旧来レイアウトを踏襲。値はDTOから供給。
   NOTE:
   * The `stats_base` must be exactly 12 bytes as per your current client usage.
   * 現行クライアント想定では `stats_base` は12バイトであること。
----------------------------------------------------------------------------- */
inline Packet to_packet(const dto::CharacterStatsInfo& info)
{
    Packet p;
    proto::put16(p, static_cast<uint16_t>(proto::ServerPacket::StatsInfo));

    auto putv = [&](uint16_t v) { proto::put16(p, v); };

    // Header/points & 6 base stats + 6 flags/bytes (exact layout)
    // 頭/ポイント & 6基本ステータス+6バイト（レイアウト厳守）
    putv(info.points_free);
    p.insert(p.end(), info.stats_base.begin(), info.stats_base.end());

    // Attack speed and misc visible stats
    // 攻撃速度などの可視ステータス
    putv(info.aspd1);
    putv(info.aspd2);

    // Vital resources & economy/weight
    // 体力/精神などの資源 & 経済/重量
    putv(info.hp);
    putv(info.max_hp);
    putv(info.sp);
    putv(info.zeny);
    putv(info.weight);
    putv(info.weight_max);

    // Movement and progression
    // 移動と成長
    putv(info.walk_speed);
    putv(info.job_level);
    putv(info.base_level);
    putv(info.status_points);

    // Offense/Defense block
    // 攻防ブロック
    putv(info.attack_min);
    putv(info.attack_max);
    putv(info.magic_attack_min);
    putv(info.magic_attack_max);
    putv(info.defense);
    putv(info.magic_defense);
    putv(info.hit);
    putv(info.flee);
    putv(info.critical);
    putv(info.karma);

    return p;
}

}  // namespace arkan::thanatos::interface::ro::mappers
