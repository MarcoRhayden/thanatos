#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace arkan::poseidon::interface::query::wire
{

// Framing: [u16 size][u16 msg_id][payload...], little-endian
enum : std::uint16_t
{
    MSG_POSEIDON_QUERY = 1,
    MSG_POSEIDON_REPLY = 2,
};

inline void w16(std::vector<std::uint8_t>& out, std::uint16_t v)
{
    out.push_back(static_cast<std::uint8_t>(v & 0xFF));
    out.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
}

inline std::uint16_t r16(const std::uint8_t* p)
{
    return static_cast<std::uint16_t>(p[0] | (p[1] << 8));
}

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

// payload: [u16 blob_size][blob...]
inline std::vector<std::uint8_t> encode_blob(std::span<const std::uint8_t> blob)
{
    std::vector<std::uint8_t> pld;

    pld.reserve(2 + blob.size());
    w16(pld, static_cast<std::uint16_t>(blob.size()));
    pld.insert(pld.end(), blob.begin(), blob.end());

    return pld;
}

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

}  // namespace arkan::poseidon::interface::query::wire
