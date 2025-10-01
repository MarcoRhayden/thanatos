#pragma once

#include <array>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/system/error_code.hpp>
#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include "application/ports/net/IConnectionHandler.hpp"
#include "application/ports/net/ISession.hpp"
#include "application/services/GameGuardBridge.hpp"
#include "application/state/SessionRegistry.hpp"
#include "infrastructure/config/Config.hpp"
#include "interface/ragnarok/model/PhaseSignal.hpp"
#include "interface/ragnarok/wire/SessionWire.hpp"
#include "shared/Hex.hpp"

namespace arkan
{
namespace thanatos
{
namespace interface
{
namespace ro
{

namespace ports_net = arkan::thanatos::application::ports::net;
using arkan::thanatos::application::state::SessionRegistry;
namespace apc = arkan::thanatos::infrastructure::config;
namespace hex = arkan::thanatos::shared::hex;

// Config/State
struct CharConfig
{
    std::array<uint8_t, 4> mapIp{127, 0, 0, 1};
    uint16_t mapPortLE{6900};
    uint16_t charBlockSize{155};
    std::string initialMap{"new_1-1.gat"};
    uint8_t sex{1};
};

struct CharState
{
    std::array<uint8_t, 4> accountID{0x81, 0x84, 0x1E, 0x00};
    std::array<uint8_t, 4> selectedCharID{0xE9, 0x03, 0x00, 0x00};
};

// Forward declare the flow to avoid heavy dependency on the header.
namespace charflow
{
class CharFlow;
}

class CharHandler : public ports_net::IConnectionHandler
{
   public:
    using OnLogFn = std::function<void(const std::string&)>;

    CharHandler(std::shared_ptr<SessionRegistry> /*registry*/, const apc::Config& cfg,
                OnLogFn logger = {});
    ~CharHandler();

    // Injects the GameGuard orchestrator
    void set_gg_bridge(arkan::thanatos::application::services::GameGuardBridge* gg)
    {
        gg_ = gg;
    }

    void on_connect(std::shared_ptr<ports_net::ISession> s) override;
    void on_disconnect(std::shared_ptr<ports_net::ISession> s, const std::error_code& ec) override;
    void on_data(std::shared_ptr<ports_net::ISession> s,
                 std::span<const std::uint8_t> bytes) override;

   private:
    void log(const std::string& m);
    void sendBytes(const std::vector<uint8_t>& p);

    std::weak_ptr<ports_net::ISession> cur_;
    CharConfig cfg_;
    CharState st_;
    OnLogFn log_;

    std::unique_ptr<charflow::CharFlow> flow_;

    // bridge to GG + session adapter
    arkan::thanatos::application::services::GameGuardBridge* gg_{nullptr};
    std::unique_ptr<wire::SessionWire> wire_;
};

}  // namespace ro
}  // namespace interface
}  // namespace thanatos
}  // namespace arkan
