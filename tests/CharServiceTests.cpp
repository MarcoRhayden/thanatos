#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

#include "application/services/ICharService.hpp"
#include "domain/protocol/Packet.hpp"
#include "infrastructure/config/Config.hpp"

using arkan::poseidon::application::services::ICharService;
using arkan::poseidon::application::services::MakeCharService;
using arkan::poseidon::domain::protocol::Packet;
using arkan::poseidon::infrastructure::config::Config;

static constexpr std::uint16_t OPC_CHAR_LIST = 0x0200;
static constexpr std::uint16_t OPC_CHAR_SELECT = 0x0201;
static constexpr std::uint16_t OPC_CHAR_OK = 0x0202;

TEST(CharService, ReturnsOneDummyCharacter)
{
    Config cfg;
    cfg.dummy_char_name = "Novice";

    auto svc = MakeCharService(cfg);

    Packet req;
    req.opcode = OPC_CHAR_LIST;

    auto out = svc->handle(req);
    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0].opcode, OPC_CHAR_LIST);

    std::string name(out[0].payload.begin(), out[0].payload.end());
    EXPECT_EQ(name, cfg.dummy_char_name);
}

TEST(CharService, SelectRedirectsToRO)
{
    Config cfg;
    cfg.ro_port = 5121;

    auto svc = MakeCharService(cfg);

    Packet req;
    req.opcode = OPC_CHAR_SELECT;

    auto out = svc->handle(req);
    ASSERT_EQ(out.size(), 1u);
    ASSERT_EQ(out[0].opcode, OPC_CHAR_OK);

    ASSERT_GE(out[0].payload.size(), 6u);
    const auto lo = static_cast<unsigned>(out[0].payload[4]);
    const auto hi = static_cast<unsigned>(out[0].payload[5]);
    const unsigned port = lo | (hi << 8);
    EXPECT_EQ(port, cfg.ro_port);
}
