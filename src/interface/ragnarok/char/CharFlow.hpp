#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "interface/ragnarok/char/CharHandler.hpp"
#include "interface/ragnarok/dto/CharacterDTO.hpp"
#include "interface/ragnarok/protocol/Codec.hpp"
#include "shared/Hex.hpp"

namespace arkan
{
namespace thanatos
{
namespace interface
{
namespace ro
{
namespace charflow
{

using arkan::thanatos::interface::ro::CharConfig;
using arkan::thanatos::interface::ro::CharState;
using arkan::thanatos::interface::ro::protocol::Packet;
namespace hex = arkan::thanatos::shared::hex;
namespace dto = arkan::thanatos::interface::ro::dto;

struct SpawnOverride
{
    std::string map;
    std::uint16_t x{0}, y{0};
    std::uint8_t dir{0};
};

class CharFlow
{
   public:
    using SendFn = std::function<void(const Packet&)>;
    using LogFn = std::function<void(const std::string&)>;

    CharFlow(const CharConfig& cfg, CharState& st, SendFn send, LogFn log);

    // Single handler: receives opcode/payload and decides what to do.
    void handle(uint16_t opcode, const uint8_t* data, size_t len);
    void armExpectingMapLogin();
    void setFixedSpawn(std::string map, std::uint16_t x, std::uint16_t y, std::uint8_t dir = 0);

   private:
    // sorting helpers
    static bool isSelectServerOpcode(uint16_t op);
    static bool isCharSelectOpcode(uint16_t op);
    static bool isEnterOpcode(uint16_t op);

    // Shared demo CharList DTO used by select-server and list-request paths.
    dto::CharListInfo makeDemoCharList() const;

    // flow steps
    void onServerSelected(const uint8_t* data, size_t len);
    void onCharListReq();
    void onCharSelected(const uint8_t* data, size_t len);
    void onEnter(uint16_t opcode, const uint8_t* data, size_t len);

    const CharConfig& cfg_;
    CharState& st_;
    SendFn send_;
    LogFn log_;

    bool awaiting_charlist_req_ = false;
    bool expecting_map_login_ = false;
    bool first_packet_after_redirect_ = false;

    std::optional<SpawnOverride> spawn_override_;
};

}  // namespace charflow
}  // namespace ro
}  // namespace interface
}  // namespace thanatos
}  // namespace arkan
