#include "CharFlow.hpp"

#include <cstring>
#include <string>

#include "interface/ragnarok/model/SpawnTable.hpp"
#include "interface/ragnarok/proto/Codec.hpp"
#include "interface/ragnarok/proto/Coords.hpp"
#include "interface/ragnarok/proto/Messages.hpp"

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

using namespace arkan::thanatos::interface::ro::proto;
using namespace arkan::thanatos::interface::ro::proto::msg;
using arkan::thanatos::interface::ro::model::safeSpawnFor;

/* -------------------- ctor -------------------- */
CharFlow::CharFlow(CharConfig& cfg, CharState& st, SendFn send, LogFn log)
    : cfg_(cfg), st_(st), send_(std::move(send)), log_(std::move(log))
{
}

/* ----------------- classifiers ------------- */
bool CharFlow::isSelectServerOpcode(uint16_t op)
{
    switch (op)
    {
        case 0x0065:
        case 0x0275:
        case 0x0825:
        case 0x0B1D:
        case 0x1DD6:
        case 0x2B0F:
        case 0x0101:
            return true;
        default:
            return false;
    }
}

bool CharFlow::isCharSelectOpcode(uint16_t op)
{
    switch (op)
    {
        case 0x0066:
        case 0x08A9:
        case 0x0B19:
        case 0x1DD7:
        case 0x2B10:
            return true;
        default:
            return false;
    }
}

bool CharFlow::isEnterOpcode(uint16_t op)
{
    return (op == 0x2844 || op == 0x0436 || op == 0x0072 || op == 0x009B);
}

/* -------------------- handle -------------------- */
void CharFlow::handle(uint16_t opcode, const uint8_t* data, size_t len)
{
    // Robust fallback
    // treat the FIRST packet as an enter, even if the opcode is not in the list.
    if (expecting_map_login_)
    {
        if (isEnterOpcode(opcode))
        {
            onEnter(opcode, data, len);
            return;
        }
        if (first_packet_after_redirect_)
        {
            first_packet_after_redirect_ = false;
            log_(std::string("enter fallback: treating first packet as MAP-LOGIN (opcode=0x") +
                 hex::hex16(opcode) + ", len=" + std::to_string(len) + ")");
            onEnter(opcode, data, len);
            return;
        }
    }

    if (isSelectServerOpcode(opcode))
    {
        onServerSelected(data, len);
        return;
    }
    if (opcode == 0x09A1)
    {
        onCharListReq();
        return;
    }
    if (isCharSelectOpcode(opcode))
    {
        onCharSelected(data, len);
        return;
    }

    log_(std::string("unhandled opcode: 0x") + hex::hex16(opcode) + " (len=" + std::to_string(len) +
         ")");
}

void CharFlow::armExpectingMapLogin()
{
    expecting_map_login_ = true;
    first_packet_after_redirect_ = true;
    log_("armed expecting_map_login_ from CharHandler");
}

void CharFlow::setFixedSpawn(std::string map, std::uint16_t x, std::uint16_t y, std::uint8_t dir)
{
    spawn_override_ = SpawnOverride{std::move(map), x, y, dir};
    log_("fixed spawn set");
}

/* ----------------- flow steps -------------- */

void CharFlow::onServerSelected(const uint8_t* data, size_t len)
{
    log_("server selected");
    if (len >= 4) std::memcpy(st_.accountID.data(), data, 4);

    // preamble + list 099D (1 char)
    send_(RawAccountIdPreamble(st_.accountID));
    send_(Preamble082D());
    send_(Preamble09A0(1));
    send_(CharList099D_Block155(
        /*cidLE*/ 1001,
        /*name*/ "Rhayden",
        /*map*/ cfg_.initialMap,
        /*male*/ cfg_.sex ? true : false));
    awaiting_charlist_req_ = true;
}

void CharFlow::onCharListReq()
{
    if (awaiting_charlist_req_)
    {
        log_("got 09A1 -> resend 099D");
        awaiting_charlist_req_ = false;
        send_(CharList099D_Block155(1001, "Rhayden", cfg_.initialMap, cfg_.sex ? true : false));
    }
    else
    {
        log_("got 09A1 but not awaiting; ignored");
    }
}

void CharFlow::onCharSelected(const uint8_t* data, size_t len)
{
    log_("character selected -> 0071+0AC5");
    if (len >= 4) std::memcpy(st_.selectedCharID.data(), data, 4);

    send_(Redirect0071(st_.selectedCharID, cfg_.initialMap, cfg_.mapIp, cfg_.mapPortLE));

    const std::array<uint8_t, 4> loginA{0x00, 0x5E, 0xD0, 0xB2};
    const std::array<uint8_t, 4> loginB{0xFF, 0x00, 0x00, 0x00};
    send_(Redirect0AC5(st_.accountID, st_.selectedCharID, loginA, loginB, cfg_.sex, cfg_.mapIp,
                       cfg_.mapPortLE, cfg_.initialMap));

    setFixedSpawn(/*map*/ cfg_.initialMap, /*x*/ 49, /*y*/ 113, /*dir*/ 0);
    armExpectingMapLogin();
}

void CharFlow::onEnter(uint16_t opcode, const uint8_t* data, size_t len)
{
    (void)opcode;
    expecting_map_login_ = false;
    first_packet_after_redirect_ = false;

    // Safe spawn + (optional) use client A3
    std::string map = cfg_.initialMap;
    uint16_t x = safeSpawnFor(cfg_.initialMap).x;
    uint16_t y = safeSpawnFor(cfg_.initialMap).y;
    uint8_t dir = safeSpawnFor(cfg_.initialMap).dir;

    // If there is explicit override
    if (spawn_override_)
    {
        map = spawn_override_->map;
        x = spawn_override_->x;
        y = spawn_override_->y;
        dir = spawn_override_->dir;
        spawn_override_.reset();
        log_("MAP-LOGIN (fixed) -> map=" + map + " x=" + std::to_string(x) +
             " y=" + std::to_string(y) + " dir=" + std::to_string((int)dir));
    }
    // Otherwise, try A3 from the client (if it comes), keeping its current behavior
    else if (len >= 19)
    {
        auto a3 = proto::decodeA3(data + 16);
        a3 = proto::clamp1023(a3);
        if (a3.x || a3.y)
        {
            x = a3.x;
            y = a3.y;
            dir = a3.dir;
        }
        log_("MAP-LOGIN (client A3) -> x=" + std::to_string(x) + " y=" + std::to_string(y) +
             " dir=" + std::to_string((int)dir));
    }

    log_(std::string("MAP-LOGIN -> x=") + std::to_string(x) + " y=" + std::to_string(y) +
         " dir=" + std::to_string((int)dir));

    // Stable sequence
    send_(SyncAccount(st_.accountID));
    send_(MapLoaded02EB(tick_ms(), x, y, dir));
    send_(AcceptEnter0073(x, y, dir));
    send_(Warp0091(cfg_.initialMap, x, y));

    // Minimal HUD
    send_(AttackRange013A(1));
    send_(Stats00BD());
    send_(HpSp00B0(40, 40, 9, 11));
    send_(LoadConfirm0B1B());
    send_(LookTo009C(st_.accountID, 4));
    send_(SystemChat009A("Welcome to Thanatos!"));
    send_(ActorInfoSelf0A30(st_.accountID, "Rhayden"));
    send_(ActorNameSelf0095(st_.accountID, "Rhayden"));
}

}  // namespace charflow
}  // namespace ro
}  // namespace interface
}  // namespace thanatos
}  // namespace arkan
