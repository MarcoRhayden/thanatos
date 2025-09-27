#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include "application/services/ILoginService.hpp"
#include "domain/protocol/Packet.hpp"
#include "infrastructure/config/Config.hpp"

using arkan::poseidon::application::services::ILoginService;
using arkan::poseidon::application::services::MakeLoginService;
using arkan::poseidon::domain::protocol::Packet;
using arkan::poseidon::infrastructure::config::Config;

static constexpr std::uint16_t OPC_LOGIN_REQ = 0x0100;
static constexpr std::uint16_t OPC_LOGIN_OK = 0x0101;
static constexpr std::uint16_t OPC_SERVER_LIST = 0x0102;

TEST(LoginService, AcceptsAnyCredentialAndReturnsServerList)
{
    Config cfg;
    cfg.char_port = 6121;

    auto svc = MakeLoginService(cfg);

    Packet in;
    in.opcode = OPC_LOGIN_REQ;
    in.payload = {'u', 's', 'e', 'r', 0, 'p', 'a', 's', 's'};

    auto out = svc->handle(in);
    ASSERT_EQ(out.size(), 2u);

    EXPECT_EQ(out[0].opcode, OPC_LOGIN_OK);

    ASSERT_EQ(out[1].opcode, OPC_SERVER_LIST);
    // payload: 4 bytes IP, 2 bytes port (LE), name
    ASSERT_GE(out[1].payload.size(), 6u);
    const auto lo = static_cast<unsigned>(out[1].payload[4]);
    const auto hi = static_cast<unsigned>(out[1].payload[5]);
    const unsigned port = lo | (hi << 8);
    EXPECT_EQ(port, cfg.char_port);
}
