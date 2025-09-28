#include <gtest/gtest.h>

#include <cstdint>
#include <span>
#include <vector>

#include "interface/query/QueryProtocol.hpp"

namespace wire = arkan::poseidon::interface::query::wire;

TEST(QueryProtocol, FrameAndBlob)
{
    const std::uint8_t payload_raw[] = {0xDE, 0xAD, 0xBE, 0xEF};
    std::span<const std::uint8_t> payload(payload_raw, sizeof(payload_raw));

    auto framed = wire::frame(wire::MSG_POSEIDON_QUERY, payload);
    ASSERT_GE(framed.size(), 4u);

    const auto size = wire::r16(framed.data());
    const auto msg_id = wire::r16(framed.data() + 2);
    EXPECT_EQ(size, framed.size());
    EXPECT_EQ(msg_id, wire::MSG_POSEIDON_QUERY);

    std::vector<std::uint8_t> blob;
    auto pld = std::span<const std::uint8_t>(framed.data() + 4, framed.size() - 4);

    // prepare blob payload and decode back
    auto blob_pld = wire::encode_blob(payload);
    ASSERT_TRUE(
        wire::decode_blob(std::span<const std::uint8_t>(blob_pld.data(), blob_pld.size()), blob));
    ASSERT_EQ(blob.size(), payload.size());
    for (size_t i = 0; i < blob.size(); ++i) EXPECT_EQ(blob[i], payload[i]);
}
