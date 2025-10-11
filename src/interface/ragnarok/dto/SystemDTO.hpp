#pragma once

#include <string>

namespace arkan::thanatos::interface::ro::dto
{

// -----------------------------------------------------------------------------
// SystemChatMessage → ServerPacket::SystemChat (0x009A)
// Plain server-to-client chat line shown in the system chat window.
// The mapper will emit: [opcode=0x009A][len][utf8 bytes][NUL].
// Keep messages short and UTF-8 clean; client renders raw text.
// サーバからクライアントのシステムチャット欄へ表示する単純な文字列。
// マッパー側で [opcode=0x009A][長さ][UTF-8本体][NUL] を生成します。
// 文字列は短め・UTF-8厳守が安全。クライアントは生文字列をそのまま描画します。
// -----------------------------------------------------------------------------
struct SystemChatMessage
{
    std::string message;  // Text to display (UTF-8, no color codes by default)
                          // 表示するテキスト（UTF-8、基本は色情報なし）
};

}  // namespace arkan::thanatos::interface::ro::dto
