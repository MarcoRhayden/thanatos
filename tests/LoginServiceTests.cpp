#include <gtest/gtest.h>

#include "application/services/ILoginService.hpp"
#include "domain/protocol/Codec.hpp"
#include "domain/protocol/Packet.hpp"
#include "domain/protocol/PacketIds.hpp"
#include "infrastructure/config/Config.hpp"

namespace proto = arkan::poseidon::domain::protocol;
namespace ids = arkan::poseidon::domain::protocol::ids;
namespace cfgns = arkan::poseidon::infrastructure::config;
using arkan::poseidon::application::services::MakeLoginService;

TEST(LoginService, AcceptsAnyCredentialAndReturnsServerListInOnePacket)
{
    cfgns::Config cfg;
    cfg.char_port = 6121;

    auto svc = MakeLoginService(cfg);

    proto::Packet in;
    in.opcode = ids::C_LOGIN_REQ;

    auto out = svc->handle(in);

    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0].opcode, ids::S_LOGIN_OK_SERVER_LIST);

    ASSERT_GE(out[0].payload.size(), 2u);
    const auto count = static_cast<uint16_t>(out[0].payload[0] | (out[0].payload[1] << 8));
    EXPECT_GE(count, 1u);
}
