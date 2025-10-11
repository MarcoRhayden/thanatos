#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

namespace arkan::thanatos::interface::query::bus
{

// ============================================================================
// Bus value model and message frame
// - This "bus" is a tiny, self-describing envelope used between processes.
// - Payloads can be a map (key→Value) or an array (Value[]), selected by
//   the `options` flag. Each Value carries its own type tag.
// 小さな自己記述エンベロープ（プロセス間バス）。
// - ペイロードは `options` により map 形式または array 形式を選択。
// - Value は型タグを持ち、個々に自己記述となる。
// ============================================================================

// Primitive types transported on the bus.
// バス上で運ぶプリミティブ型。
enum class ValueType : uint8_t
{
    Binary = 0,  // arbitrary bytes / 任意バイト列
    String = 1,  // raw UTF-8 stored in `bin` / UTF-8 を bin に格納
    UInt = 2     // unsigned big-endian integer / 符号なし整数（ビッグエンディアン）
};

// Variant-like container for a single value.
// 1 つの値を保持するバリアント風のコンテナ。
struct Value
{
    ValueType type;
    std::vector<uint8_t> bin;  // for Binary and String / Binary と String 用
    uint64_t u = 0;            // for UInt / UInt 用
};

// A bus message envelope.
// - `options == 0` -> use args_map
// - `options == 1` -> use args_array
// `options` の値でペイロード形式を切り替える。
struct Message
{
    uint8_t options = 0;    // 0: map, 1: array
    std::string messageID;  // short identifier / 短い識別子
    std::unordered_map<std::string, Value> args_map;
    std::vector<Value> args_array;
};

// ============================================================================
// Big-endian helpers (frame header is BE).
// フレームヘッダはビッグエンディアン。ユーティリティ群。
// ============================================================================

inline void wr_be32(std::vector<uint8_t>& b, uint32_t x)
{
    b.push_back(uint8_t((x >> 24) & 0xFF));
    b.push_back(uint8_t((x >> 16) & 0xFF));
    b.push_back(uint8_t((x >> 8) & 0xFF));
    b.push_back(uint8_t(x & 0xFF));
}

inline void wr_be24(std::vector<uint8_t>& b, uint32_t x)
{
    b.push_back(uint8_t((x >> 16) & 0xFF));
    b.push_back(uint8_t((x >> 8) & 0xFF));
    b.push_back(uint8_t(x & 0xFF));
}

inline uint32_t rd_be32(const uint8_t* p)
{
    return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) | (uint32_t(p[2]) << 8) | uint32_t(p[3]);
}

inline uint32_t rd_be24(const uint8_t* p)
{
    return (uint32_t(p[0]) << 16) | (uint32_t(p[1]) << 8) | uint32_t(p[2]);
}

// ============================================================================
// Encoding
// - Frame layout (big-endian):
//   [len:be32][options:u8][mid_len:u8][MID:bytes][entries...]
// - Map entries:
//   repeat { [key_len:u8][key][type:u8][val_len:be24][val_bytes...] }
// - Array entries:
//   repeat { [type:u8][val_len:be24][val_bytes...] }
// 変換仕様（BE）。
// ============================================================================

inline std::vector<uint8_t> encode(const Message& m)
{
    if (m.messageID.size() > 255) throw std::runtime_error("MID too long");
    if (m.options != 0 && m.options != 1) throw std::runtime_error("options must be 0 or 1");

    std::vector<uint8_t> out;
    out.reserve(64);

    // Reserve 4 bytes for total length; fill later.
    // 総バイト数は後で上書き。
    wr_be32(out, 0);
    out.push_back(m.options);
    out.push_back(static_cast<uint8_t>(m.messageID.size()));
    out.insert(out.end(), m.messageID.begin(), m.messageID.end());

    if (m.options == 0)
    {
        // ---- Map form / マップ形式 ----
        for (const auto& [k, v] : m.args_map)
        {
            if (k.size() > 255) throw std::runtime_error("key too long");
            out.push_back(static_cast<uint8_t>(k.size()));
            out.insert(out.end(), k.begin(), k.end());

            out.push_back(static_cast<uint8_t>(v.type));

            if (v.type == ValueType::String || v.type == ValueType::Binary)
            {
                if (v.bin.size() > 0xFFFFFF) throw std::runtime_error("binary/string too large");
                wr_be24(out, static_cast<uint32_t>(v.bin.size()));
                out.insert(out.end(), v.bin.begin(), v.bin.end());
            }
            else
            {
                // UInt → write minimal-length big-endian (no leading zeros, min 1 byte).
                // 先頭ゼロを除いた最小長の BE で出力（最低 1 バイト）。
                uint8_t tmp[8];
                int n = 0;
                uint64_t x = v.u;
                do
                {
                    tmp[7 - n] = uint8_t(x & 0xFF);
                    x >>= 8;
                    n++;
                } while (x && n < 8);

                wr_be24(out, n);
                out.insert(out.end(), &tmp[8 - n], &tmp[8]);
            }
        }
    }
    else
    {
        // ---- Array form / 配列形式 ----
        for (const auto& v : m.args_array)
        {
            out.push_back(static_cast<uint8_t>(v.type));

            if (v.type == ValueType::String || v.type == ValueType::Binary)
            {
                if (v.bin.size() > 0xFFFFFF) throw std::runtime_error("array val too large");
                wr_be24(out, static_cast<uint32_t>(v.bin.size()));
                out.insert(out.end(), v.bin.begin(), v.bin.end());
            }
            else
            {
                uint8_t tmp[8];
                int n = 0;
                uint64_t x = v.u;
                do
                {
                    tmp[7 - n] = uint8_t(x & 0xFF);
                    x >>= 8;
                    n++;
                } while (x && n < 8);

                wr_be24(out, n);
                out.insert(out.end(), &tmp[8 - n], &tmp[8]);
            }
        }
    }

    // Patch total length at the head (big-endian).
    // 先頭に総長（BE）を書き戻す。
    const uint32_t len = static_cast<uint32_t>(out.size());
    out[0] = uint8_t((len >> 24) & 0xFF);
    out[1] = uint8_t((len >> 16) & 0xFF);
    out[2] = uint8_t((len >> 8) & 0xFF);
    out[3] = uint8_t(len & 0xFF);

    return out;
}

// ============================================================================
// Decoding
// - Returns the number of bytes consumed (== frame length).
// - Throws std::runtime_error on malformed frames.
// 復号。消費したバイト数（フレーム長）を返す。不正なフレームは例外。
// ============================================================================

inline size_t decode(const uint8_t* data, size_t n, Message& m_out)
{
    if (n < 6) throw std::runtime_error("buffer too small");

    const uint32_t len = rd_be32(data);
    if (len > n) throw std::runtime_error("incomplete frame");

    const uint8_t options = data[4];
    const uint8_t mid_len = data[5];
    if (6u + mid_len > len) throw std::runtime_error("bad MID length");

    // Reset output and copy header fields.
    // 出力を初期化し、ヘッダを反映。
    m_out = {};
    m_out.options = options;
    m_out.messageID.assign(reinterpret_cast<const char*>(data + 6), mid_len);

    size_t off = 6 + mid_len;

    if (options == 0)
    {
        // ---- Map form / マップ形式 ----
        while (off < len)
        {
            if (off + 1 > len) throw std::runtime_error("truncated map key_len");
            uint8_t klen = data[off++];

            if (off + klen + 1 + 3 > len) throw std::runtime_error("truncated map entry");
            std::string key(reinterpret_cast<const char*>(data + off), klen);
            off += klen;

            Value v{};
            v.type = static_cast<ValueType>(data[off++]);

            const uint32_t vlen = rd_be24(data + off);
            off += 3;
            if (off + vlen > len) throw std::runtime_error("truncated map value");

            if (v.type == ValueType::String || v.type == ValueType::Binary)
            {
                v.bin.assign(data + off, data + off + vlen);
            }
            else
            {
                // Parse big-endian unsigned integer (variable length).
                // 可変長 BE の符号なし整数を復元。
                uint64_t x = 0;
                for (uint32_t i = 0; i < vlen; ++i) x = (x << 8) | data[off + i];
                v.u = x;
            }
            off += vlen;

            m_out.args_map.emplace(std::move(key), std::move(v));
        }
    }
    else if (options == 1)
    {
        // ---- Array form / 配列形式 ----
        while (off < len)
        {
            if (off + 1 + 3 > len) throw std::runtime_error("truncated array entry");

            Value v{};
            v.type = static_cast<ValueType>(data[off++]);

            const uint32_t vlen = rd_be24(data + off);
            off += 3;
            if (off + vlen > len) throw std::runtime_error("truncated array value");

            if (v.type == ValueType::String || v.type == ValueType::Binary)
            {
                v.bin.assign(data + off, data + off + vlen);
            }
            else
            {
                uint64_t x = 0;
                for (uint32_t i = 0; i < vlen; ++i) x = (x << 8) | data[off + i];
                v.u = x;
            }
            off += vlen;

            m_out.args_array.emplace_back(std::move(v));
        }
    }
    else
    {
        // Only 0 (map) and 1 (array) are valid.
        // 0（map）と 1（array）以外は不正。
        throw std::runtime_error("invalid options");
    }

    // Return the exact number of bytes the caller should consume from its buffer.
    // 呼び出し側がバッファから消費すべき正確なバイト数を返す。
    return len;
}

}  // namespace arkan::thanatos::interface::query::bus
