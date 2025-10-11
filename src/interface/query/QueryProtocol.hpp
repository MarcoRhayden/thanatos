#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace arkan::thanatos::interface::query::wire
{

// -----------------------------------------------------------------------------
// Lightweight length-prefixed framing for local Query/Reply messages.
// Frame layout (little-endian):
//   [u16 size][u16 msg_id][payload...]
// - size includes the 4 bytes of (size + msg_id) plus payload bytes.
// - msg_id is a small enum defined below.
// - Intended for in-process or LAN usage; no CRC/checksums by design.
// ローカルの Query/Reply メッセージ向けの軽量な長さ付きフレーミング。
// フレーム構造（リトルエンディアン）:
//   [u16 size][u16 msg_id][payload...]
// ・size は先頭 4 バイト（size + msg_id）を含む総バイト数。
// ・msg_id は下記の小さな列挙で表現。
// ・プロセス内/同一 LAN 想定のため CRC/チェックサムは設けない。
enum : std::uint16_t
{
    MSG_THANATOS_QUERY = 1,  // Client -> Server: carries an encoded payload
                             // クライアント→サーバ: エンコード済みのペイロードを運ぶ
    MSG_THANATOS_REPLY = 2,  // Server -> Client: response payload
                             // サーバ→クライアント: 応答のペイロード
};

// -----------------------------------------------------------------------------
// Write a 16-bit little-endian value into a byte vector.
// out is appended; its existing contents are preserved.
// 16 ビット値をリトルエンディアンでベクタ末尾へ書き込む。
// out は追記され、既存内容は保持される。
inline void w16(std::vector<std::uint8_t>& out, std::uint16_t v)
{
    out.push_back(static_cast<std::uint8_t>(v & 0xFF));
    out.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
}

// -----------------------------------------------------------------------------
// Read a 16-bit little-endian value from a raw pointer.
// Caller must ensure p points to at least 2 bytes.
// 生ポインタから 16 ビット（LE）を読み取る。
// 呼び出し側は p が少なくとも 2 バイトを指すことを保証する。
inline std::uint16_t r16(const std::uint8_t* p)
{
    return static_cast<std::uint16_t>(p[0] | (p[1] << 8));
}

// -----------------------------------------------------------------------------
// Build a framed message: [size][msg_id][payload...].
// - size is computed automatically as 4 + payload.size().
// - payload is copied into the resulting vector.
// - Cost: O(N) where N = payload.size().
// フレーム付きメッセージを構築する: [size][msg_id][payload...]。
// ・size は自動計算（4 + payload.size()）
// ・payload は結果ベクタへコピーされる
// ・計算量: O(N)（N は payload.size()）
inline std::vector<std::uint8_t> frame(std::uint16_t msg_id, std::span<const std::uint8_t> payload)
{
    const std::uint16_t size = static_cast<std::uint16_t>(4u + payload.size());

    std::vector<std::uint8_t> out;
    out.reserve(size);

    w16(out, size);
    w16(out, msg_id);

    out.insert(out.end(), payload.begin(), payload.end());

    return out;
}

// -----------------------------------------------------------------------------
// Encode a binary blob as a length-prefixed payload:
//   [u16 blob_size][blob...]
// Useful as the inner payload for frame(), keeping parsing trivial.
// バイナリを長さ付きペイロードとしてエンコード:
//   [u16 blob_size][blob...]
// frame() の内側ペイロードとして使うとパースが単純になる。
inline std::vector<std::uint8_t> encode_blob(std::span<const std::uint8_t> blob)
{
    std::vector<std::uint8_t> pld;

    pld.reserve(2 + blob.size());
    w16(pld, static_cast<std::uint16_t>(blob.size()));
    pld.insert(pld.end(), blob.begin(), blob.end());

    return pld;
}

// -----------------------------------------------------------------------------
// Decode a length-prefixed blob from a payload produced by encode_blob().
// Returns true on success and fills 'out' with the blob bytes. On failure,
// returns false and leaves 'out' empty.
// Safety: Validates that the announced size fits within 'payload'.
// 長さ付きブロブを decode。成功で true を返し、out に内容を格納。
// 失敗時は false を返し、out は空のまま。
// 安全性: 指定サイズが payload の範囲内か検証する。
inline bool decode_blob(std::span<const std::uint8_t> payload, std::vector<std::uint8_t>& out)
{
    out.clear();

    if (payload.size() < 2) return false;

    const auto n = static_cast<std::uint16_t>(payload[0] | (payload[1] << 8));

    size_t pos = 2;
    if (pos + n > payload.size()) return false;

    out.insert(out.end(), payload.begin() + static_cast<std::ptrdiff_t>(pos),
               payload.begin() + static_cast<std::ptrdiff_t>(pos + n));

    return true;
}

}  // namespace arkan::thanatos::interface::query::wire
