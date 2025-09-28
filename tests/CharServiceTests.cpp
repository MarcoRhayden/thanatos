#include <gtest/gtest.h>

#include "application/services/ICharService.hpp"
#include "domain/protocol/Codec.hpp"
#include "domain/protocol/Packet.hpp"
#include "domain/protocol/PacketIds.hpp"
#include "infrastructure/config/Config.hpp"

namespace proto = arkan::poseidon::domain::protocol;
namespace ids = arkan::poseidon::domain::protocol::ids;
namespace cfgns = arkan::poseidon::infrastructure::config;
using arkan::poseidon::application::services::MakeCharService;

TEST(CharService, ReturnsOneDummyCharacter)
{
    cfgns::Config cfg;
    cfg.dummy_char_name = "Novice";
    cfg.dummy_char_map = "new_zone01";
    cfg.dummy_char_x = 53;
    cfg.dummy_char_y = 111;

    auto svc = MakeCharService(cfg);

    proto::Packet in;
    in.opcode = ids::C_CHAR_LIST_REQ;

    auto out = svc->handle(in);

    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0].opcode, ids::S_CHAR_LIST);

    ASSERT_GE(out[0].payload.size(), 2u);
    const auto count = static_cast<uint16_t>(out[0].payload[0] | (out[0].payload[1] << 8));
    EXPECT_GE(count, 1u);
}

TEST(CharService, SelectRedirectsToRO)
{
    cfgns::Config cfg;
    cfg.ro_host = "127.0.0.1";
    cfg.ro_port = 5121;
    cfg.dummy_char_map = "new_zone01";
    cfg.dummy_char_x = 53;
    cfg.dummy_char_y = 111;

    auto svc = MakeCharService(cfg);

    proto::Packet in;
    in.opcode = ids::C_CHAR_SELECT;

    auto out = svc->handle(in);

    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0].opcode, ids::S_CONNECT_TO_MAP);
}
