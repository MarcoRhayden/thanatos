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

// ===== Bus Types (SSM)
enum class ValueType : uint8_t
{
    Binary = 0,
    String = 1,
    UInt = 2
};

struct Value
{
    ValueType type;
    std::vector<uint8_t> bin;  // used for Binary and String (raw UTF-8)
    uint64_t u = 0;            // used for UInt
};

struct Message
{
    // options = 0 -> valid args_map; = 1 -> valid args_array
    uint8_t options = 0;
    std::string messageID;
    std::unordered_map<std::string, Value> args_map;
    std::vector<Value> args_array;
};

// ===== Utils (big-endian / u24)
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

// ===== Encode ===== //
inline std::vector<uint8_t> encode(const Message& m)
{
    if (m.messageID.size() > 255) throw std::runtime_error("MID too long");
    if (m.options != 0 && m.options != 1) throw std::runtime_error("options must be 0 or 1");

    std::vector<uint8_t> out;
    out.reserve(64);

    // length placeholder
    wr_be32(out, 0);
    out.push_back(m.options);
    out.push_back(static_cast<uint8_t>(m.messageID.size()));
    out.insert(out.end(), m.messageID.begin(), m.messageID.end());

    if (m.options == 0)
    {
        // Map entries
        for (const auto& [k, v] : m.args_map)
        {
            if (k.size() > 255) throw std::runtime_error("key too long");
            out.push_back(static_cast<uint8_t>(k.size()));
            out.insert(out.end(), k.begin(), k.end());

            out.push_back(static_cast<uint8_t>(v.type));
            if (v.type == ValueType::String)
            {
                if (v.bin.size() > 0xFFFFFF) throw std::runtime_error("string too large");
                wr_be24(out, static_cast<uint32_t>(v.bin.size()));
                out.insert(out.end(), v.bin.begin(), v.bin.end());
            }
            else if (v.type == ValueType::Binary)
            {
                if (v.bin.size() > 0xFFFFFF) throw std::runtime_error("binary too large");
                wr_be24(out, static_cast<uint32_t>(v.bin.size()));
                out.insert(out.end(), v.bin.begin(), v.bin.end());
            }
            else
            {
                // UInt -> we encode as big-endian without leading zeros (minimum 1 byte)
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
        // Array entries
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

    // Fills length (total size)
    const uint32_t len = static_cast<uint32_t>(out.size());

    out[0] = uint8_t((len >> 24) & 0xFF);
    out[1] = uint8_t((len >> 16) & 0xFF);
    out[2] = uint8_t((len >> 8) & 0xFF);
    out[3] = uint8_t(len & 0xFF);

    return out;
}

// ===== Decode (returns consumed bytes; throws on format error) ===== //
inline size_t decode(const uint8_t* data, size_t n, Message& m_out)
{
    if (n < 6) throw std::runtime_error("buffer too small");

    const uint32_t len = rd_be32(data);

    if (len > n) throw std::runtime_error("incomplete frame");

    const uint8_t options = data[4];
    const uint8_t mid_len = data[5];

    if (6u + mid_len > len) throw std::runtime_error("bad MID length");

    m_out = {};
    m_out.options = options;
    m_out.messageID.assign(reinterpret_cast<const char*>(data + 6), mid_len);

    size_t off = 6 + mid_len;
    if (options == 0)
    {
        // map
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
                // UInt (big-endian sem sinal)
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
        throw std::runtime_error("invalid options");
    }
    return len;  // bytes consumidos
}

}  // namespace arkan::thanatos::interface::query::bus
