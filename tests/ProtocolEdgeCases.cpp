#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "domain/protocol/Codec.hpp"
#include "domain/protocol/Packet.hpp"
#include "domain/protocol/Parser.hpp"

namespace proto = arkan::poseidon::domain::protocol;

static proto::Packet MakePacket(uint16_t opcode, const std::string& s)
{
    proto::Packet p;
    p.opcode = opcode;
    p.payload.assign(s.begin(), s.end());
    return p;
}

TEST(Protocol, FragmentationAndCoalescing)
{
    proto::Parser parser;

    auto p1 = MakePacket(0x0064, "hello");
    auto p2 = MakePacket(0x0065, "world!");

    auto b1 = proto::Encode(p1);
    auto b2 = proto::Encode(p2);

    // fragmented feed (half of the first)
    std::vector<std::uint8_t> f1(b1.begin(), b1.begin() + b1.size() / 2);
    parser.feed(f1);
    auto out = parser.drain();
    EXPECT_TRUE(out.empty());

    // complete the first one and send the second one all at once
    std::vector<std::uint8_t> f2(b1.begin() + b1.size() / 2, b1.end());
    f2.insert(f2.end(), b2.begin(), b2.end());
    parser.feed(f2);

    out = parser.drain();
    ASSERT_EQ(out.size(), 2u);
    EXPECT_EQ(out[0].opcode, p1.opcode);
    EXPECT_EQ(std::string(out[0].payload.begin(), out[0].payload.end()), "hello");
    EXPECT_EQ(out[1].opcode, p2.opcode);
    EXPECT_EQ(std::string(out[1].payload.begin(), out[1].payload.end()), "world!");
}