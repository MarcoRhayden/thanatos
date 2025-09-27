#include <gtest/gtest.h>

#include <cstdint>
#include <span>
#include <vector>

#include "domain/protocol/Codec.hpp"
#include "domain/protocol/Packet.hpp"
#include "domain/protocol/Parser.hpp"

using namespace arkan::poseidon::domain::protocol;

TEST(Protocol, EncodeHeader)
{
    Packet p{0x1234, {0xAA, 0xBB, 0xCC}};
    auto buf = Encode(p);
    ASSERT_EQ(buf.size(), 4u + 3u);
    // size = 7 (LE)
    EXPECT_EQ(buf[0], 0x34);
    EXPECT_EQ(buf[1], 0x12);
    EXPECT_EQ(buf[2], 0x07);
    EXPECT_EQ(buf[3], 0x00);
    EXPECT_EQ(buf[4], 0xAA);
    EXPECT_EQ(buf[6], 0xCC);
}

TEST(Protocol, ParserHandlesFragmentation)
{
    Packet p{0x0101, {1, 2, 3, 4, 5}};
    auto buf = Encode(p);

    Parser parser;
    parser.feed(std::span<const std::uint8_t>(buf.data(), 3));
    auto out = parser.drain();
    EXPECT_TRUE(out.empty());

    parser.feed(std::span<const std::uint8_t>(buf.data() + 3, buf.size() - 3));
    out = parser.drain();
    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0].opcode, 0x0101);
    EXPECT_EQ(out[0].payload, p.payload);
}

TEST(Protocol, ParserDropsInvalidAndRecovers)
{
    // invalid packet: size < 4 (will clear the buffer)
    std::vector<std::uint8_t> invalid = {0x00, 0x01, 0x03, 0x00};  // opcode=0x0100, size=3
    Packet p{0x0202, {9, 8, 7}};
    auto valid = Encode(p);

    Parser parser;
    parser.feed(invalid);
    auto o1 = parser.drain();
    EXPECT_TRUE(o1.empty());

    parser.feed(valid);
    auto o2 = parser.drain();
    ASSERT_EQ(o2.size(), 1u);
    EXPECT_EQ(o2[0].opcode, 0x0202);
    EXPECT_EQ(o2[0].payload, p.payload);
}
