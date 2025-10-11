#pragma once

#include "interface/ragnarok/dto/SystemDTO.hpp"
#include "interface/ragnarok/protocol/Codec.hpp"
#include "interface/ragnarok/protocol/Opcodes.hpp"

namespace arkan::thanatos::interface::ro::mappers
{

namespace proto = arkan::thanatos::interface::ro::protocol;
namespace dto = arkan::thanatos::interface::ro::dto;
using proto::Packet;

/* -----------------------------------------------------------------------------
   0x009A SystemChat
   - Sends a server/system message to the client chat log.
   - サーバ／システムメッセージをクライアントのチャットログへ送信する。
   - Layout: [u16 opcode][u16 length][bytes message][u8 NUL]
   - フォーマット: [u16 opcode][u16 length][bytes message][u8 NUL]
   - `length` must include the 4-byte header and the NUL terminator.
   - `length` は4バイトのヘッダ＋末尾のNULも含めた総サイズ。
----------------------------------------------------------------------------- */
inline Packet to_packet(const dto::SystemChatMessage& info)
{
    Packet p;

    // Opcode / オペコード
    proto::put16(p, static_cast<uint16_t>(proto::ServerPacket::SystemChat));

    // Total packet size = header (2 + 2) + message + NUL
    // 総パケットサイズ = ヘッダ(2+2) + 本文 + NUL
    const uint16_t len = static_cast<uint16_t>(4 + info.message.size() + 1);
    proto::put16(p, len);

    // Payload bytes of the message (no implicit NUL here)
    // メッセージ本文の生バイト（ここでは自動NUL付与なし）
    p.insert(p.end(), info.message.begin(), info.message.end());

    // Explicit NUL terminator required by the protocol
    // プロトコル上、明示的なNUL終端が必要
    p.push_back(0);

    return p;
}

}  // namespace arkan::thanatos::interface::ro::mappers
